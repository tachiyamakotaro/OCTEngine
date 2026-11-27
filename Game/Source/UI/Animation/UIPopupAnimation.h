/**
 * @file UIPopupAnimation.h
 * @brief ウィンドウを「ポンと大きくなって出る・小さくなって消える」ように出し入れするアニメーション
 * @details 状態：閉じている → Open → 開いている途中 → 開いている → Close → 閉じている途中 → 閉じている
 *            開く … OPEN_TIME 秒で、少し小さい所から少し行きすぎて 1 に戻る（EaseOutBack）。だんだん濃くなる
 *            閉じる … CLOSE_TIME 秒で、少し小さくなりながら消える（EaseIn）
 *          今の拡大率と濃さは MakeTransform（UIGroupTransform）で受け取って、ウィンドウと中身を置くのに使う。
 *          閉じている途中も IsVisible は true なので、持ち主は閉じきるまで描き続ける。
 *          毎フレーム Update を呼ぶ（開いている・閉じている間は何もしない）。
 *          ウィンドウ以外の UI にも使えるように、何を動かすかは知らない（拡大率と濃さを計算するだけ）。
 */
#pragma once
#include "Source/UI/UIGroupTransform.h"


namespace app
{
	namespace ui
	{
		/**
		 * @brief 拡大縮小で出し入れするアニメーション
		 */
		class UIPopupAnimation
		{
		public:
			UIPopupAnimation();

			/** 開き始める（閉じている途中でも、今の大きさから開く） */
			void Open();
			/** 閉じ始める（開いている途中でも、今の大きさから閉じる） */
			void Close();
			/** すぐに閉じた状態にする（アニメーションしない） */
			void Hide();
			/** 時間を進める（毎フレーム呼ぶ） */
			void Update();

			/** 見えているか（開いている・開いている途中・閉じている途中） */
			bool IsVisible() const { return m_state != State::Closed; }
			/** 開ききっているか */
			bool IsOpened() const { return m_state == State::Opened; }
			/**
			 * @brief 今の拡大率と濃さ
			 * @param pivot 拡大の中心（ふつうはウィンドウの真ん中）
			 */
			UIGroupTransform MakeTransform(const Vector2& pivot) const;


		private:
			/** 状態 */
			enum class State
			{
				Closed,		//!< 閉じている（描かない）
				Opening,	//!< 開いている途中
				Opened,		//!< 開いている
				Closing,	//!< 閉じている途中
			};

			/** 今の拡大率 */
			float GetScale() const;
			/** 今の濃さ */
			float GetAlpha() const;


		private:
			State m_state;
			/** 今の状態になってからの割合（0 〜 1） */
			float m_rate;
			/** 閉じ始めたときの拡大率と濃さ（開いている途中で閉じても、そこから小さくなる） */
			float m_closeFromScale;
			float m_closeFromAlpha;
		};
	}
}
