/**
 * @file HiddenFloor.h
 * @brief 暗がりの足場（光で照らすと見える足場）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		class DarkZone;


		/**
		 * @brief 暗がりの足場
		 * @details ・光の線分の近く（距離が「見える距離」以内）にある間、見えるようになる。
		 *            見える距離は、足場がある暗がり（DarkZone）の lightRadius。暗がりの外なら revealDistance。
		 *          ・光が離れても stayTime 秒は見えたまま、そのあと fadeTime 秒かけて見えなくなる。
		 *          ・isSolidWhenHidden が true なら、見えなくてもそこにある（乗れる）。false なら見えている間だけ乗れる。
		 *          ・光は止めない（光は足場を通り抜けて、奥まで照らす）。
		 *          ・暗がりの isPlayerRevealingFloors が true なら、プレイヤーのまわりの丸の中に入っても見えるようになる。
		 *            （プレイヤーの体は、Update に渡される体の箱 context.bodies で調べる）
		 */
		class HiddenFloor : public RectGimmick
		{
		public:
			/** 見えなくても乗れるか（false なら見えている間だけ乗れる） */
			bool isSolidWhenHidden = true;
			/** 光が離れてから、見えたままでいる時間（秒） */
			float stayTime = 1.5f;
			/** 見えなくなるまでにかける時間（秒） */
			float fadeTime = 0.5f;
			/** 見えるようになるまでにかける時間（秒） */
			float appearTime = 0.15f;
			/** 暗がりの外に置いたときの、見える距離（暗がりの中なら、その暗がりの lightRadius を使う） */
			float revealDistance = 120.0f;

			/** 【実行時】見え具合（0=見えない 〜 1=はっきり見える） */
			float visibility = 0.0f;
			/** 【実行時】今フレーム光に照らされているか */
			bool isLitNow = false;
			/** 【実行時】光が離れてからの時間 */
			float darkTimer = 0.0f;
			/** 【実行時】このフレームに見え始めたか（音や演出のきっかけに使う） */
			bool isRevealedThisFrame = false;


		public:
			const char* GetTypeName() const override { return "HiddenFloor"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<HiddenFloor>(*this); }

			/** 光の計算の結果を見る */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 見えているか */
			bool IsVisible() const { return visibility > 0.0f; }
			/** 乗れるか */
			bool IsSolid() const { return isSolidWhenHidden || IsVisible(); }
			/** 見える距離（足場の中心がある暗がりの lightRadius。暗がりの外なら revealDistance） */
			float GetRevealDistance(const GimmickManager& gimmicks) const;
			/** 足場の中心がある暗がり（なければ nullptr） */
			const DarkZone* FindZone(const GimmickManager& gimmicks) const;


		public:
			/** x, y は左下 */
			HiddenFloor(const char* name, float x, float y, float width, float height);
		};


		/** @brief 線分と矩形の距離（重なっていれば 0） */
		float DistanceSegmentToRect(const math::Vec2& a, const math::Vec2& b, const math::Rect& rect);
	}
}
