/**
 * @file MenuInput.cpp
 * @brief メニューのカーソル操作
 */
#include "stdafx.h"
#include "MenuInput.h"


namespace
{
	/** スティックを倒したとみなす値と、戻したとみなす値 */
	constexpr float STICK_ON = 0.6f;
	constexpr float STICK_OFF = 0.3f;
}


namespace app
{
	namespace ui
	{
		void MenuInput::Reset()
		{
			m_isStickUp = m_isStickDown = m_isStickLeft = m_isStickRight = true;
			m_stickUpTrigger = m_stickDownTrigger = m_stickLeftTrigger = m_stickRightTrigger = false;
		}


		void MenuInput::Update()
		{
			// 倒した瞬間だけ true（戻すまでは次の入力にならない）
			auto edge = [](const float value, bool& isHeld, bool& trigger)
			{
				trigger = false;
				if (!isHeld && value >= STICK_ON)
				{
					isHeld = true;
					trigger = true;
				}
				else if (isHeld && value <= STICK_OFF)
				{
					isHeld = false;
				}
			};
			const float x = g_pad[0]->GetLStickXF();
			const float y = g_pad[0]->GetLStickYF();
			edge(y, m_isStickUp, m_stickUpTrigger);
			edge(-y, m_isStickDown, m_stickDownTrigger);
			edge(-x, m_isStickLeft, m_stickLeftTrigger);
			edge(x, m_isStickRight, m_stickRightTrigger);
		}


		bool MenuInput::IsUp() const { return g_pad[0]->IsTrigger(enButtonUp) || m_stickUpTrigger; }
		bool MenuInput::IsDown() const { return g_pad[0]->IsTrigger(enButtonDown) || m_stickDownTrigger; }
		bool MenuInput::IsLeft() const { return g_pad[0]->IsTrigger(enButtonLeft) || m_stickLeftTrigger; }
		bool MenuInput::IsRight() const { return g_pad[0]->IsTrigger(enButtonRight) || m_stickRightTrigger; }
		bool MenuInput::IsDecide() const { return g_pad[0]->IsTrigger(enButtonA); }
		bool MenuInput::IsCancel() const { return g_pad[0]->IsTrigger(enButtonB); }
	}
}
