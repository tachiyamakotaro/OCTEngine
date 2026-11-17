#pragma once


/**
 * @file GoalEditor.h
 * @brief ゴールの門とクリアの演出を調整する ImGui（デバッグウィンドウの「Goal」の中身）
 */
namespace app
{
	namespace core
	{
		class IrisWipe;
		struct IrisStyle;
	}

	namespace stage
	{
		class ClearSequence;
		class GoalGateRenderer;
		struct ClearTiming;
		struct GateStyle;


		/**
		 * @brief ゴールの門とクリアの演出を調整する ImGui
		 * @details ・GoalGateRenderer / ClearSequence / IrisWipe は ImGui を使わない。ImGui の項目はこのクラスにまとめる
		 *            （シーンに書くのは何もない。InGameEditor が Draw を1行呼ぶだけ）
		 *          ・ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 *          ・項目ごとの関数には、その調整値（ClearTiming など）だけを渡す
		 */
		class GoalEditor
		{
		public:
			GoalEditor() = default;
			~GoalEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param sequence クリアの演出の流れ（時間）
			 * @param renderer 門の描画（扉・光・炎）
			 * @param iris 丸く閉じるフェード
			 */
			void Draw(ClearSequence& sequence, GoalGateRenderer& renderer, core::IrisWipe& iris);

		private:
			/** クリアの演出の時間 */
			void DrawTiming(ClearTiming& timing);
			/** 門の見た目（扉・光・炎） */
			void DrawGate(GateStyle& style);
			/** 丸く閉じるフェード */
			void DrawIris(core::IrisStyle& style);
		};
	}
}
