/**
 * @file InvisibleFloor.h
 * @brief 透明の足場（周りの風景を映していて見えない足場）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 透明の足場
		 * @details 霧が触れると表面に露がついて見えるようになり、霧が晴れても露が残るので見えたまま。
		 *          isSolidWhenHidden が true なら、見えないうちから乗れる（本当にそこにある足場）。
		 *          false にすると、見えるようになるまで当たり判定がない。
		 */
		class InvisibleFloor : public RectGimmick
		{
		public:
			/** 見えないうちから当たり判定があるか */
			bool isSolidWhenHidden = true;

			/** 【実行時】霧に触れて見えるようになったか（一度見えたら見えたまま） */
			bool isRevealed = false;
			/** 【実行時】見えるようになってからの時間（露の演出用） */
			float revealedTime = 0.0f;
			/** 【実行時】このフレームに見えるようになったか（エフェクトや音を出すきっかけに使う） */
			bool isRevealedThisFrame = false;


		public:
			const char* GetTypeName() const override { return "InvisibleFloor"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<InvisibleFloor>(*this); }

			/** 霧が動いたあとに調べる */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Late; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 当たり判定があるか */
			bool IsSolid() const { return isRevealed || isSolidWhenHidden; }


		public:
			/** x, y は左下 */
			InvisibleFloor(const char* name, float x, float y, float width, float height);
		};
	}
}
