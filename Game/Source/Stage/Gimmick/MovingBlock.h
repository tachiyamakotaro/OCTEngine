/**
 * @file MovingBlock.h
 * @brief ソーラーパネル付きの動く床・動く壁（パネルに光をためると、ためた分だけ動く）
 */
#pragma once
#include "MovePath.h"
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ソーラーパネル付きの動く床・動く壁
		 * @details 床か壁かは大きさだけの違い（横長なら床、縦長なら壁）。エディタの R キーで回すと、動く向きとパネルの面も一緒に回る。
		 *          ・ブロックの1つの面にソーラーパネルが付いている。パネルに光が当たっている間、電気がたまる（charge が増える）。
		 *          ・満タンになると動き出し、終点へ向かう。終点に着いたらそこで止まって待つ。
		 *            動いている間（終点で待っている間も）電気は減っていき、なくなると始点へ戻る。
		 *            光が当たり続けていれば電気は減らないので、ずっと動いたまま（終点にいたまま）になる。
		 *          ・パネルの色は、ためている間はたまった量に合わせて黒 → 灰色 → 白に変わり、動いている間は白く光ったまま（電気が残り少なくなると白 → 黒に戻っていく）（StageRenderer が描く）。
		 *          ・rect は「置いた位置」（始点）。動いても rect は変えず、今の位置は GetCurrentRect() で求める。
		 *          ・乗っているキャラクターは、床が動いた分だけ一緒に運ばれる（GetCarryDelta）。
		 *          ・体を地形との間にはさんでしまうときは、その間だけ止まる（押しつぶさない）。
		 *          ・当たり判定は地形あつかい。光はパネルに当たったら吸収（電気になる）、パネル以外の面では止まる。
		 */
		class MovingBlock : public RectGimmick
		{
		public:
			/**
			 * @brief ソーラーパネルを付ける面
			 */
			enum class PanelSide : int
			{
				Top = 0,	//!< 上の面
				Bottom = 1,	//!< 下の面
				Left = 2,	//!< 左の面
				Right = 3,	//!< 右の面
				Num
			};


		public:
			/** 動き方（終点までのずれ・速さ） */
			MovePath movePath;
			/** ソーラーパネルを付ける面 */
			PanelSide panelSide = PanelSide::Top;
			/** パネルの長さ（面に沿った長さ。0 なら面いっぱい。面より長ければ面いっぱい） */
			float panelLength = 0.0f;
			/**
			 * 手前の面（カメラに見えている面）全体にもパネルを貼るか（見た目だけ。見やすくするため）
			 * 光を受けるのは panelSide の面のまま。手前のパネルも、panelSide のパネルと同じように光る
			 */
			bool hasFrontPanel = false;
			/** 空から満タンまで、光を当て続ける時間（秒） */
			float chargeTime = 2.0f;
			/** 満タンから空まで、動いていられる時間（秒） */
			float runTime = 4.0f;

			/** 【実行時】たまった電気（0=空 〜 1=満タン） */
			float charge = 0.0f;
			/** 【実行時】動いているか（満タンになったら true、空になったら false） */
			bool isRunning = false;
			/** 【実行時】パネルに光が当たっているか */
			bool isLit = false;
			/** 【実行時】このフレームで動く前の範囲（乗っているキャラクターを運ぶのに使う） */
			math::Rect previousRect;


		public:
			const char* GetTypeName() const override { return "MovingBlock"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<MovingBlock>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;
			bool GetCarryDelta(const math::Rect& groundProbe, math::Vec2& outDelta) const override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

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
			/** 今のソーラーパネルの範囲（ブロックの面の内側の薄い帯） */
			math::Rect GetPanelRect() const { return CalcPanelRect(GetCurrentRect()); }
			/**
			 * @brief パネルの光り具合（0=黒 〜 1=白く光る）
			 * @details ためている間はたまった量。
			 *          動いている間は白く光ったままで、電気が PANEL_FADE_CHARGE より少なくなったら、空に向かって白 → 黒に戻っていく
			 */
			float GetPanelBrightness() const
			{
				if (!isRunning)
				{
					return charge;
				}
				return math::Min(charge / PANEL_FADE_CHARGE, 1.0f);
			}


		public:
			/** パネルの厚さ（面から内側へ） */
			static constexpr float PANEL_THICKNESS = 10.0f;
			/** 動いている間、電気がこれより少なくなったらパネルが暗くなり始める（0〜1。0.25 なら残り 1/4 から） */
			static constexpr float PANEL_FADE_CHARGE = 0.25f;


		public:
			/** x, y は始点の左下 */
			MovingBlock(const char* name, float x, float y, float width, float height);


		private:
			/** 矩形をずらす */
			static math::Rect Translate(const math::Rect& source, const math::Vec2& delta) { return { source.min + delta, source.max + delta }; }
			/** ブロックの範囲 block に付いているパネルの範囲 */
			math::Rect CalcPanelRect(const math::Rect& block) const;
			/** 電気をためる・使う（動いてよいかを決める） */
			void UpdateCharge(float deltaTime);
			/** 体を delta だけ押したとき、ほかの地形にはさまれるか */
			bool IsBlockedByBody(const GimmickUpdateContext& context, const math::Rect& nextRect, const math::Vec2& delta) const;
		};
	}
}
