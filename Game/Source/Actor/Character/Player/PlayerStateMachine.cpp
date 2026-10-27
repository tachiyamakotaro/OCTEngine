/**
 * @file PlayerStateMachine.cpp
 * @brief プレイヤーのステートマシン
 */
#include "stdafx.h"
#include "PlayerStateMachine.h"
#include "Player.h"
#include "PlayerParameter.h"
#include "PlayerState.h"
#include "Source/Actor/Character/CharacterStatus.h"


namespace app
{
	namespace actor
	{
		Player& PlayerStateMachine::GetPlayer() const
		{
			return *m_player;
		}


		const PlayerParameter& PlayerStateMachine::GetParam() const
		{
			return m_player->GetPlayerParameter();
		}


		const CharacterCommand& PlayerStateMachine::GetCommand() const
		{
			return m_player->GetCommand();
		}


		CharacterController& PlayerStateMachine::GetController() const
		{
			return m_player->GetController();
		}


		void PlayerStateMachine::ChangeState(const PlayerStateID stateID)
		{
			CharacterStateMachine::ChangeState(static_cast<int>(stateID));
		}


		PlayerStateID PlayerStateMachine::GetCurrentPlayerStateID() const
		{
			return static_cast<PlayerStateID>(GetCurrentStateID());
		}


		void PlayerStateMachine::ConsumeJump()
		{
			m_jumpBufferTimer = 0.0f;
			m_coyoteTimer = 0.0f;
		}


		bool PlayerStateMachine::WantsToGrabLedge() const
		{
			if (m_ledgeCooldownTimer > 0.0f)
			{
				return false;
			}
			const CharacterCommand& command = GetCommand();
			// 下を押している間はつかまない（わざと下に落ちたいとき用）
			if (command.down)
			{
				return false;
			}
			return GetParam().isAutoLedgeGrab || command.grabHold;
		}


		bool PlayerStateMachine::TryFindLedge()
		{
			const PlayerParameter& param = GetParam();
			LedgeQuery query;
			query.reach = param.ledgeGrabReach;
			query.above = param.ledgeGrabAbove;
			query.below = param.ledgeGrabBelow;
			return GetController().FindLedge(m_player->GetFacing(), query, m_ledge);
		}


		void PlayerStateMachine::StartLedgeCooldown()
		{
			m_ledgeCooldownTimer = GetParam().ledgeRegrabCooldown;
		}


		float PlayerStateMachine::GetFallDistance() const
		{
			const float distance = m_airPeakY - GetController().GetPosition().y;
			return distance > 0.0f ? distance : 0.0f;
		}


		void PlayerStateMachine::UpdateFacingFromInput()
		{
			const CharacterCommand& command = GetCommand();
			if (command.HasMoveInput())
			{
				m_player->SetFacing(command.moveX > 0.0f ? 1 : -1);
			}
		}


		void PlayerStateMachine::ApplyGroundMove(const float deltaTime)
		{
			const PlayerParameter& param = GetParam();
			ApplyHorizontalMove(GetCommand().moveX, param.runSpeed, param.groundAcceleration, param.groundDeceleration, deltaTime);
		}


		void PlayerStateMachine::ApplyAirMove(const float deltaTime)
		{
			const PlayerParameter& param = GetParam();
			ApplyHorizontalMove(GetCommand().moveX, param.runSpeed, param.airAcceleration, param.airDeceleration, deltaTime);
		}


		void PlayerStateMachine::ApplyStop(const float deltaTime)
		{
			const PlayerParameter& param = GetParam();
			ApplyHorizontalMove(0.0f, param.runSpeed, param.groundAcceleration, param.groundDeceleration, deltaTime);
		}


		void PlayerStateMachine::ApplyGrabMove(const float deltaTime)
		{
			const PlayerParameter& param = GetParam();
			ApplyHorizontalMove(GetCommand().moveX, param.grabMoveSpeed, param.groundAcceleration, param.groundDeceleration, deltaTime);
		}


		void PlayerStateMachine::Setup()
		{
			RegisterState(static_cast<int>(PlayerStateID::Idle), std::make_unique<PlayerIdleState>(this));
			RegisterState(static_cast<int>(PlayerStateID::Run), std::make_unique<PlayerRunState>(this));
			RegisterState(static_cast<int>(PlayerStateID::Crouch), std::make_unique<PlayerCrouchState>(this));
			RegisterState(static_cast<int>(PlayerStateID::Jump), std::make_unique<PlayerJumpState>(this));
			RegisterState(static_cast<int>(PlayerStateID::Fall), std::make_unique<PlayerFallState>(this));
			RegisterState(static_cast<int>(PlayerStateID::LedgeHang), std::make_unique<PlayerLedgeHangState>(this));
			RegisterState(static_cast<int>(PlayerStateID::ClimbUp), std::make_unique<PlayerClimbUpState>(this));
			RegisterState(static_cast<int>(PlayerStateID::HardLand), std::make_unique<PlayerHardLandState>(this));
			RegisterState(static_cast<int>(PlayerStateID::Dead), std::make_unique<PlayerDeadState>(this));
			RegisterState(static_cast<int>(PlayerStateID::Grab), std::make_unique<PlayerGrabState>(this));

			m_airPeakY = GetController().GetPosition().y;
			ChangeState(PlayerStateID::Idle);
		}


		int PlayerStateMachine::CheckGlobalTransition()
		{
			// HPが0になったら、どのステートからでも死亡へ
			if (m_player->GetStatus()->IsDead())
			{
				return static_cast<int>(PlayerStateID::Dead);
			}
			return STATE_ID_NONE;
		}


		void PlayerStateMachine::PreUpdate(const float deltaTime)
		{
			const PlayerParameter& param = GetParam();
			CharacterController& controller = GetController();

			// しゃがみ以外で体が縮んだままなら元に戻す（空中でしゃがみが解除された時の保険）
			if (GetCurrentPlayerStateID() != PlayerStateID::Crouch && controller.GetHeight() < param.bodyHeight)
			{
				controller.TryResizeHeight(param.bodyHeight);
			}

			// ジャンプの先行入力
			if (GetCommand().jumpTrigger)
			{
				m_jumpBufferTimer = param.jumpBufferTime;
			}
			else if (m_jumpBufferTimer > 0.0f)
			{
				m_jumpBufferTimer -= deltaTime;
			}

			// コヨーテタイム（地面にいる間は満タンにしておき、離れたら減らす）
			// ジャンプした直後のフレームはまだ接地判定が残っているので、上昇中は満タンにしない
			if (controller.IsOnGround() && controller.GetVelocity().y <= 0.0f)
			{
				m_coyoteTimer = param.coyoteTime;
			}
			else if (m_coyoteTimer > 0.0f)
			{
				m_coyoteTimer -= deltaTime;
			}

			// 崖つかまりのクールダウン
			if (m_ledgeCooldownTimer > 0.0f)
			{
				m_ledgeCooldownTimer -= deltaTime;
			}

			// 落下距離の基準点：地面や崖（重力OFF）にいる間は今の高さ、空中では最高点を記録する
			const float y = controller.GetPosition().y;
			if (controller.IsOnGround() || !controller.IsGravityEnabled())
			{
				m_airPeakY = y;
			}
			else if (y > m_airPeakY)
			{
				m_airPeakY = y;
			}
		}


		PlayerStateMachine::PlayerStateMachine(Player* owner)
			: CharacterStateMachine(owner)
			, m_player(owner)
			, m_jumpBufferTimer(0.0f)
			, m_coyoteTimer(0.0f)
			, m_ledgeCooldownTimer(0.0f)
			, m_ledge()
			, m_airPeakY(0.0f)
		{}
	}
}
