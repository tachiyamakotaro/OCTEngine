/**
 * @file GamePadInputDevice.cpp
 * @brief OCTEngine(k2EngineLow) の g_pad を読む入力デバイス
 */
#include "stdafx.h"
#include "GamePadInputDevice.h"


namespace app
{
	namespace input
	{
		namespace
		{
			/** スティックを「倒している」とみなすしきい値 */
			constexpr float STICK_THRESHOLD = 0.5f;
			/** アナログ入力の遊び（これ以下は0として扱う） */
			constexpr float STICK_DEADZONE = 0.2f;
		}


		GamePadInputDevice::GamePadInputDevice(const int padNo)
			: m_padNo(padNo)
		{}


		bool GamePadInputDevice::IsPress(const InputAction action) const
		{
			const GamePad* pad = g_pad[m_padNo];
			switch (action)
			{
			case InputAction::Left:		return pad->IsPress(enButtonLeft) || pad->GetLStickXF() < -STICK_THRESHOLD;
			case InputAction::Right:	return pad->IsPress(enButtonRight) || pad->GetLStickXF() > STICK_THRESHOLD;
			case InputAction::Up:		return pad->IsPress(enButtonUp) || pad->GetLStickYF() > STICK_THRESHOLD;
			case InputAction::Down:		return pad->IsPress(enButtonDown) || pad->GetLStickYF() < -STICK_THRESHOLD;
			case InputAction::Jump:		return pad->IsPress(enButtonA);
			case InputAction::Grab:		return pad->IsPress(enButtonRB1);
			case InputAction::Action:	return pad->IsPress(enButtonB);
			default:					return false;
			}
		}


		float GamePadInputDevice::GetMoveX() const
		{
			const GamePad* pad = g_pad[m_padNo];

			// 十字キーが押されていればデジタル入力を優先
			if (pad->IsPress(enButtonLeft) || pad->IsPress(enButtonRight))
			{
				return IInputDevice::GetMoveX();
			}

			const float stickX = pad->GetLStickXF();
			if (stickX > -STICK_DEADZONE && stickX < STICK_DEADZONE)
			{
				return 0.0f;
			}
			return stickX;
		}
	}
}
