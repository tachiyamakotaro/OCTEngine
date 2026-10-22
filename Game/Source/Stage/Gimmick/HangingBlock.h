/**
 * @file HangingBlock.h
 * @brief 吊るされた板（つないだロープが燃え尽きると落ちて、下の地形に乗る）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 吊るされた板
		 * @details ・rect は吊るされている位置（落ちてもこの値は変わらない。今の位置は GetCurrentRect）。
		 *          ・ropes に入れた信号（ロープの名前）のどれか1つでも ON（燃え尽きた）になると、重力で落ち始める。
		 *          ・下にある地形（ブロック・草・ほかの板など）に当たったら、その上に乗って止まる。
		 *            穴の両側の床より幅を広くしておくと、落ちて穴にかかる橋になる。
		 *          ・キャラクターの上に落ちてきたら、その頭の上で止まって待つ（つぶさない）。どいたらまた落ちる。
		 *          ・吊るされている間も落ちている間も地形（Terrain）で、光を止める。
		 */
		class HangingBlock : public RectGimmick
		{
		public:
			/** 吊るしているロープの名前（どれか1本でも燃え尽きたら落ちる） */
			std::vector<std::string> ropes;
			/** 重力（1秒あたりに増える落ちる速さ） */
			float gravity = 2500.0f;
			/** 落ちる速さの上限 */
			float maxFallSpeed = 1800.0f;

			/** 【実行時】吊るされている位置から、どれだけ落ちたか */
			float fallDistance = 0.0f;
			/** 【実行時】落ちる速さ */
			float fallSpeed = 0.0f;
			/** 【実行時】落ちているか（ロープが切れてから、乗るまで） */
			bool isFalling = false;
			/** 【実行時】地形に乗って止まったか */
			bool isLanded = false;
			/** 【実行時】今フレーム、ロープが切れて落ち始めたか */
			bool isReleasedThisFrame = false;
			/** 【実行時】今フレーム、地形に乗ったか（エフェクト・SE 用） */
			bool isLandedThisFrame = false;


		public:
			const char* GetTypeName() const override { return "HangingBlock"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<HangingBlock>(*this); }

			/** ロープ（燃える物）は Late で燃え尽きるので、その次のフレームの Signal で落ち始める */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Signal; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			void RenameSignal(const std::string& oldName, const std::string& newName) override;
			void RemoveSignal(const std::string& signalName) override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 今の位置 */
			math::Rect GetCurrentRect() const
			{
				math::Rect current = rect;
				current.min.y -= fallDistance;
				current.max.y -= fallDistance;
				return current;
			}


		private:
			/**
			 * @brief 1フレームぶん落とす
			 * @details 今の位置から下へ動かした範囲に、地形・キャラクターの体があれば、その上で止める
			 */
			void Fall(const GimmickUpdateContext& context);


		public:
			/** x, y は吊るされているときの左下 */
			HangingBlock(const char* name, float x, float y, float width, float height);
		};
	}
}
