/**
 * @file GamePadInputDevice.h
 * @brief OCTEngine(k2EngineLow) の g_pad を読む入力デバイス
 */
#pragma once
#include "IInputDevice.h"


namespace app
{
	namespace input
	{
		/**
		 * @brief ゲームパッド入力デバイス
		 * @details ボタン割り当て：
		 *          左右 = 左スティック or 十字キー / 上下 = 左スティック or 十字キー
		 *          A = ジャンプ / RB1 = つかむ / B = アクション
		 */
		class GamePadInputDevice : public IInputDevice
		{
		public:
			/**
			 * @param padNo 使うパッド番号（g_pad の添字）
			 */
			explicit GamePadInputDevice(int padNo = 0);


		public:
			bool IsPress(InputAction action) const override;
			float GetMoveX() const override;


		private:
			/** パッド番号 */
			int m_padNo;
		};
	}
}
