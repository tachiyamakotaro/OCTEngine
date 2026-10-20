/**
 * @file MovingBlock.h
 * @brief 動く床・動く壁（つながった信号が全部 ON の間だけ動く）
 */
#pragma once
#include "RectGimmick.h"
#include "MovePath.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 動く床・動く壁
		 * @details 床か壁かは大きさだけの違い（横長なら床、縦長なら壁）。エディタの R キーで回すと、動く向きも一緒に回る。
		 *          ・rect は「置いた位置」（始点）。動いても rect は変えず、今の位置は GetCurrentRect() で求める。
		 *          ・つながった信号（ソーラーパネル・受光器・スイッチ）が全部 ON の間だけ、movePath に沿って動く。
		 *            OFF になるとその場で止まる。つながりがなければずっと止まっている。
		 *          ・乗っているキャラクターは、床が動いた分だけ一緒に運ばれる（GetCarryDelta）。
		 *          ・体を地形との間にはさんでしまうときは、その間だけ止まる（押しつぶさない）。
		 *          ・当たり判定は地形あつかい。光も止める。
		 */
		class MovingBlock : public RectGimmick
		{
		public:
			/** 動くのに必要な信号の名前（ソーラーパネル・受光器・スイッチ。全部 ON で動く） */
			std::vector<std::string> requiredSignals;
			/** 動き方（終点までのずれ・速さ・往復か片道か） */
			MovePath movePath;

			/** 【実行時】このフレームで動く前の範囲（乗っているキャラクターを運ぶのに使う） */
			math::Rect previousRect;
			/** 【実行時】動いてよい状態か（信号が全部 ON） */
			bool isPowered = false;


		public:
			const char* GetTypeName() const override { return "MovingBlock"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<MovingBlock>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Signal; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;
			bool GetCarryDelta(const math::Rect& groundProbe, math::Vec2& outDelta) const override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			void RenameSignal(const std::string& oldName, const std::string& newName) override;
			void RemoveSignal(const std::string& signalName) override;

			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Rotate90(int quarterTurns) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 今の範囲（置いた位置 + 動いた分） */
			math::Rect GetCurrentRect() const { return Translate(rect, movePath.GetOffset()); }
			/** 終点に着いたときの範囲（エディタで道のりを見せるのに使う） */
			math::Rect GetEndRect() const { return Translate(rect, movePath.offset); }


		public:
			/** x, y は始点の左下 */
			MovingBlock(const char* name, float x, float y, float width, float height);


		private:
			/** 矩形をずらす */
			static math::Rect Translate(const math::Rect& source, const math::Vec2& delta) { return { source.min + delta, source.max + delta }; }
			/** 体を delta だけ押したとき、ほかの地形にはさまれるか */
			bool IsBlockedByBody(const GimmickUpdateContext& context, const math::Rect& nextRect, const math::Vec2& delta) const;
		};
	}
}
