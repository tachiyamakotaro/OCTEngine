/**
 * @file PlayerState.cpp
 * @brief プレイヤーの各ステート
 */
#include "stdafx.h"
#include "PlayerState.h"
#include "Player.h"
#include "PlayerParameter.h"
#include "PlayerStatus.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace actor
	{
		namespace
		{
			/** 「止まっている」とみなす左右速度 */
			constexpr float STOP_SPEED = 1.0f;
			/** 崖登り：上に持ち上がる区間の終わり（0〜1の割合） */
			constexpr float CLIMB_RISE_END = 0.6f;
		}


		/************************************/
		// PlayerStateBase


		void PlayerStateBase::PlayAnimation(const int animation) const
		{
			GetPlayer().PlayAnimation(animation);
		}


		int PlayerStateBase::CheckGroundCommonTransition() const
		{
			if (!GetController().IsOnGround())
			{
				return ToID(PlayerStateID::Fall);
			}
			if (m_machine->IsJumpBuffered())
			{
				return ToID(PlayerStateID::Jump);
			}
			if (GetCommand().down)
			{
				return ToID(PlayerStateID::Crouch);
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// 立ち


		void PlayerIdleState::Enter()
		{
			PlayAnimation(static_cast<int>(PlayerAnimation::Idle));
		}


		void PlayerIdleState::Update(const float deltaTime)
		{
			m_machine->UpdateFacingFromInput();
			m_machine->ApplyGroundMove(deltaTime);
		}


		int PlayerIdleState::CheckTransition()
		{
			const int common = CheckGroundCommonTransition();
			if (common != STATE_ID_NONE)
			{
				return common;
			}

			const CharacterCommand& command = GetCommand();
			if (command.HasMoveInput())
			{
				// 壁に向かって押しているだけなら走らない
				const int direction = command.moveX > 0.0f ? 1 : -1;
				if (!GetController().IsTouchingWall(direction))
				{
					return ToID(PlayerStateID::Run);
				}
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// 走り


		void PlayerRunState::Enter()
		{
			PlayAnimation(static_cast<int>(PlayerAnimation::Run));
		}


		void PlayerRunState::Update(const float deltaTime)
		{
			m_machine->UpdateFacingFromInput();
			m_machine->ApplyGroundMove(deltaTime);
		}


		int PlayerRunState::CheckTransition()
		{
			const int common = CheckGroundCommonTransition();
			if (common != STATE_ID_NONE)
			{
				return common;
			}

			const CharacterController& controller = GetController();
			// 壁にぶつかった
			if (controller.IsTouchingWall(GetPlayer().GetFacing()))
			{
				return ToID(PlayerStateID::Idle);
			}
			// 入力をやめて止まりきった
			if (!GetCommand().HasMoveInput() && math::Abs(controller.GetVelocity().x) < STOP_SPEED)
			{
				return ToID(PlayerStateID::Idle);
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// しゃがみ


		void PlayerCrouchState::Enter()
		{
			GetController().TryResizeHeight(GetParam().crouchHeight);
			PlayAnimation(static_cast<int>(PlayerAnimation::Crouch));
		}


		void PlayerCrouchState::Update(const float deltaTime)
		{
			m_machine->UpdateFacingFromInput();
			m_machine->ApplyStop(deltaTime);
		}


		void PlayerCrouchState::Exit()
		{
			GetController().TryResizeHeight(GetParam().bodyHeight);
		}


		int PlayerCrouchState::CheckTransition()
		{
			const CharacterController& controller = GetController();
			if (!controller.IsOnGround())
			{
				return ToID(PlayerStateID::Fall);
			}
			// 下を離して、頭上が空いていれば立ち上がる
			if (!GetCommand().down && controller.CanResizeHeight(GetParam().bodyHeight))
			{
				return ToID(PlayerStateID::Idle);
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// ジャンプ（上昇中）


		void PlayerJumpState::Enter()
		{
			GetController().SetVelocityY(GetParam().jumpVelocity);
			m_machine->ConsumeJump();
			m_isJumpCut = false;
			PlayAnimation(static_cast<int>(PlayerAnimation::Jump));
		}


		void PlayerJumpState::Update(const float deltaTime)
		{
			m_machine->UpdateFacingFromInput();
			m_machine->ApplyAirMove(deltaTime);

			// 上昇中にボタンを離したら上昇を弱める（小ジャンプ）
			CharacterController& controller = GetController();
			if (!m_isJumpCut && !GetCommand().jumpHold && controller.GetVelocity().y > 0.0f)
			{
				controller.SetVelocityY(controller.GetVelocity().y * GetParam().jumpCutMultiplier);
				m_isJumpCut = true;
			}
		}


		int PlayerJumpState::CheckTransition()
		{
			// 上昇中でも崖に手が届けばつかむ（真上へのジャンプで崖をつかむ、プリンス オブ ペルシャの動き）
			if (m_machine->WantsToGrabLedge() && m_machine->TryFindLedge())
			{
				return ToID(PlayerStateID::LedgeHang);
			}
			// 上昇が終わった（天井にぶつかった場合も速度が0になる）
			if (GetController().GetVelocity().y <= 0.0f)
			{
				return ToID(PlayerStateID::Fall);
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// 落下中


		void PlayerFallState::Enter()
		{
			PlayAnimation(static_cast<int>(PlayerAnimation::Fall));
		}


		void PlayerFallState::Update(const float deltaTime)
		{
			m_machine->UpdateFacingFromInput();
			m_machine->ApplyAirMove(deltaTime);
		}


		int PlayerFallState::CheckTransition()
		{
			if (m_machine->WantsToGrabLedge() && m_machine->TryFindLedge())
			{
				return ToID(PlayerStateID::LedgeHang);
			}
			// 足場から落ちた直後ならまだジャンプできる
			if (m_machine->IsJumpBuffered() && m_machine->CanCoyoteJump())
			{
				return ToID(PlayerStateID::Jump);
			}
			if (GetController().IsOnGround())
			{
				if (m_machine->GetFallDistance() >= GetParam().hardLandingHeight)
				{
					return ToID(PlayerStateID::HardLand);
				}
				return GetCommand().HasMoveInput() ? ToID(PlayerStateID::Run) : ToID(PlayerStateID::Idle);
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// 崖にぶら下がり


		void PlayerLedgeHangState::Enter()
		{
			const LedgeInfo& ledge = m_machine->GetLedge();
			CharacterController& controller = GetController();

			// 崖の位置にぴったり合わせて、重力を止める
			controller.SetGravityEnabled(false);
			controller.SetVelocity(math::Vec2::Zero());
			controller.SetPosition(ledge.hangPosition);
			GetPlayer().SetFacing(ledge.facing);

			PlayAnimation(static_cast<int>(PlayerAnimation::LedgeHang));
		}


		void PlayerLedgeHangState::Update(const float /*deltaTime*/)
		{
			GetController().SetVelocity(math::Vec2::Zero());
		}


		void PlayerLedgeHangState::Exit()
		{
			GetController().SetGravityEnabled(true);
			// 手を離した直後に同じ崖をつかみ直さないようにする
			m_machine->StartLedgeCooldown();
		}


		int PlayerLedgeHangState::CheckTransition()
		{
			const CharacterCommand& command = GetCommand();
			if (command.up || m_machine->IsJumpBuffered())
			{
				return ToID(PlayerStateID::ClimbUp);
			}
			if (command.down)
			{
				return ToID(PlayerStateID::Fall);
			}
			// 自動つかまりOFFのときは、つかむボタンを離したら落ちる
			if (!GetParam().isAutoLedgeGrab && !command.grabHold)
			{
				return ToID(PlayerStateID::Fall);
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// 崖を登る


		void PlayerClimbUpState::Enter()
		{
			CharacterController& controller = GetController();

			m_machine->ConsumeJump();
			m_startPosition = controller.GetPosition();
			m_endPosition = m_machine->GetLedge().standPosition;

			controller.SetGravityEnabled(false);
			controller.SetVelocity(math::Vec2::Zero());

			PlayAnimation(static_cast<int>(PlayerAnimation::ClimbUp));
		}


		void PlayerClimbUpState::Update(const float /*deltaTime*/)
		{
			// 前半で体を持ち上げ、後半で崖の上へ前進する（角にめり込まないように順番に動かす）
			// ※ アニメーションのルートモーションに合わせたくなったら、ここを差し替える
			const float t = math::Clamp01(GetStateTime() / GetParam().climbUpDuration);
			const float riseT = math::SmoothStep(t / CLIMB_RISE_END);
			const float forwardT = math::SmoothStep((t - CLIMB_RISE_END) / (1.0f - CLIMB_RISE_END));

			const math::Vec2 position{
				math::Lerp(m_startPosition.x, m_endPosition.x, forwardT),
				math::Lerp(m_startPosition.y, m_endPosition.y, riseT)
			};

			CharacterController& controller = GetController();
			controller.SetPosition(position);
			controller.SetVelocity(math::Vec2::Zero());
		}


		void PlayerClimbUpState::Exit()
		{
			CharacterController& controller = GetController();
			controller.SetGravityEnabled(true);
			controller.SetVelocity(math::Vec2::Zero());
		}


		int PlayerClimbUpState::CheckTransition()
		{
			if (GetStateTime() >= GetParam().climbUpDuration)
			{
				return GetCommand().HasMoveInput() ? ToID(PlayerStateID::Run) : ToID(PlayerStateID::Idle);
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// 高いところから落ちてよろける


		void PlayerHardLandState::Enter()
		{
			const PlayerParameter& param = GetParam();
			PlayerStatus* status = GetPlayer().GetStatus<PlayerStatus>();

			// 落下距離は、着地したこのフレームの間だけ正しく取れる
			if (m_machine->GetFallDistance() >= param.deadlyFallHeight)
			{
				status->Kill();
			}
			else
			{
				status->ApplyDamage(param.fallDamage);
			}

			GetController().SetVelocityX(0.0f);
			PlayAnimation(static_cast<int>(PlayerAnimation::HardLand));
		}


		void PlayerHardLandState::Update(const float deltaTime)
		{
			m_machine->ApplyStop(deltaTime);
		}


		int PlayerHardLandState::CheckTransition()
		{
			if (!GetController().IsOnGround())
			{
				return ToID(PlayerStateID::Fall);
			}
			if (GetStateTime() >= GetParam().hardLandingDuration)
			{
				return ToID(PlayerStateID::Idle);
			}
			return STATE_ID_NONE;
		}


		/************************************/
		// 死亡


		void PlayerDeadState::Enter()
		{
			GetPlayer().GetPlayerController().SetEnabled(false);
			PlayAnimation(static_cast<int>(PlayerAnimation::Dead));
		}


		void PlayerDeadState::Update(const float deltaTime)
		{
			m_machine->ApplyStop(deltaTime);
		}
	}
}
