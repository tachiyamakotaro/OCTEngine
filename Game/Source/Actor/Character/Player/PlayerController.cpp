/**
 * @file PlayerController.cpp
 * @brief プレイヤーの入力をキャラクターコマンドに変換するクラス
 */
#include "stdafx.h"
#include "PlayerController.h"
#include "Source/Input/IInputDevice.h"


namespace app
{
	namespace actor
	{
		void PlayerController::Update(CharacterCommand& outCommand)
		{
			outCommand.Clear();

			if (m_device == nullptr)
			{
				return;
			}

			using input::InputAction;
			const bool isJumpPressed = m_device->IsPress(InputAction::Jump);
			const bool isActionPressed = m_device->IsPress(InputAction::Action);

			if (m_isEnabled)
			{
				const float moveX = m_device->GetMoveX();
				outCommand.moveX = m_isMirrorX ? -moveX : moveX;
				outCommand.up = m_device->IsPress(InputAction::Up);
				outCommand.down = m_device->IsPress(InputAction::Down);
				outCommand.jumpTrigger = isJumpPressed && !m_wasJumpPressed;
				outCommand.jumpHold = isJumpPressed;
				outCommand.grabHold = m_device->IsPress(InputAction::Grab);
				outCommand.actionTrigger = isActionPressed && !m_wasActionPressed;
			}

			// 受付停止中も前フレームの状態は更新する（再開した瞬間に「押した」と誤判定しないため）
			m_wasJumpPressed = isJumpPressed;
			m_wasActionPressed = isActionPressed;
		}


		PlayerController::PlayerController(const input::IInputDevice* device)
			: m_device(device)
			, m_isMirrorX(false)
			, m_isEnabled(true)
			, m_wasJumpPressed(false)
			, m_wasActionPressed(false)
		{}
	}
}
