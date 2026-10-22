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
		 * @details ・見え具合（visibility）は、足場のいちばん近い所の明るさ（光の線分・プレイヤーのまわり）。
		 *            明るさは暗がり（DarkZone::Glow）と同じ計算なので、画面で明るく見えている所の足場が見える。
		 *            暗がりの外なら、光から revealDistance を境目の真ん中にして、revealSoftWidth の幅でぼかす。
		 *          ・光が離れたら、その場ですぐ暗くなる（時間がたつまで見えたまま、ということはない）。
		 *            境目はぼけているので、光が近づく・離れるにつれて、少しずつ見えたり消えたりする。
		 *          ・見た目は2つ：ふだん（光やプレイヤーの丸で見えている間）は暗い石のモデル。
		 *            プレイヤーが上に乗っている間だけ、明るい石のモデルにする（乗っている感じを出す）。
		 *            （前は光に近いと明るい黄色のモデルになっていたが、特別な床に見えてしまうのでやめた）
		 *          ・isSolidWhenHidden が true なら、見えなくてもそこにある（乗れる）。false なら半分以上見えている間だけ乗れる。
		 *          ・光は止めない（光は足場を通り抜けて、奥まで照らす）。
		 *          ・暗がりの isPlayerRevealingFloors が true なら、プレイヤーのまわりの丸の中に入っても見えるようになる。
		 *            （プレイヤーの体は、Update に渡される体の箱 context.bodies で調べる）
		 */
		class HiddenFloor : public RectGimmick
		{
		public:
			/** 見えなくても乗れるか（false なら半分以上見えている間だけ乗れる） */
			bool isSolidWhenHidden = true;
			/** 暗がりの外に置いたときの、見える距離（境目の真ん中。暗がりの中なら、その暗がりの lightRadius を使う） */
			float revealDistance = 120.0f;
			/** 暗がりの外に置いたときの、境目のぼかしの幅（暗がりの中なら、その暗がりの lightSoftWidth を使う） */
			float revealSoftWidth = 140.0f;

			/** 【実行時】見え具合（0=見えない 〜 1=はっきり見える） */
			float visibility = 0.0f;
			/** 【実行時】今フレーム光に照らされているか（光の線分が、境目の真ん中より近くを通っている） */
			bool isLitNow = false;
			/** 【実行時】プレイヤーが上に乗っているか（乗っている間だけ明るいモデルにする） */
			bool isPlayerOn = false;
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
			bool IsSolid() const { return isSolidWhenHidden || visibility >= 0.5f; }
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
