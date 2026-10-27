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
			// 鏡・日よけ板をつかんだ / 鏡を回した
			if (GetPlayer().IsGrabbing() || GetPlayer().IsRotatingObject())
			{
				return ToID(PlayerStateID::Grab);
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
			// つかんだ角が動く床なら、ぶら下がったまま一緒に動く
			controller.BeginLedgeCarry(ledge.corner, ledge.facing);

			PlayAnimation(static_cast<int>(PlayerAnimation::LedgeHang));
			// 見た目：手が崖の角にかかるようにモデルをずらす
			GetPlayer().SetModelOffset(math::Vec2(GetParam().ledgeHangModelForward, GetParam().ledgeHangModelY));
		}


		void PlayerLedgeHangState::Update(const float /*deltaTime*/)
		{
			GetController().SetVelocity(math::Vec2::Zero());
			// Tuning で動かした値がすぐ見えるように、毎フレーム入れ直す
			GetPlayer().SetModelOffset(math::Vec2(GetParam().ledgeHangModelForward, GetParam().ledgeHangModelY));
		}


		void PlayerLedgeHangState::Exit()
		{
			// 手を離したとき：次のアニメーションへの補間に合わせて、ずらしを少しずつ戻す
			// （崖登りに移るときは、崖登りの Enter で置き直される）
			GetPlayer().FadeOutModelOffset(
				math::Vec2(GetParam().ledgeHangModelForward, GetParam().ledgeHangModelY),
				CharacterBase::DEFAULT_ANIMATION_INTERPOLATE_TIME);
			GetController().SetGravityEnabled(true);
			GetController().EndLedgeCarry();
			// 手を離した直後に同じ崖をつかみ直さないようにする
			m_machine->StartLedgeCooldown();
		}


		int PlayerLedgeHangState::CheckTransition()
		{
			// 動く床と一緒に動けずに手が離れた（床が下りて足が地面に着いた、など）
			if (GetController().IsLedgeLost())
			{
				return ToID(PlayerStateID::Fall);
			}
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
			// ぶら下がっている間に動く床に運ばれた分だけ、登りきる位置もずらす
			const LedgeInfo& ledge = m_machine->GetLedge();
			const math::Vec2 carried = controller.GetLedgeCarryOffset();
			m_startPosition = controller.GetPosition();
			m_endPosition = ledge.standPosition + carried;
			// 登っている間も、つかんだ床と一緒に動く
			controller.BeginLedgeCarry(ledge.corner + carried, ledge.facing);

			controller.SetGravityEnabled(false);
			controller.SetVelocity(math::Vec2::Zero());

			PlayAnimation(static_cast<int>(PlayerAnimation::ClimbUp));
			UpdateClimbModel(math::Vec2::Zero());
		}


		void PlayerClimbUpState::Update(const float /*deltaTime*/)
		{
			// 前半で体を持ち上げ、後半で崖の上へ前進する（角にめり込まないように順番に動かす）
			// ※ アニメーションのルートモーションに合わせたくなったら、ここを差し替える
			const float t = math::Clamp01(GetStateTime() / GetParam().climbUpDuration);
			const float riseT = math::SmoothStep(t / CLIMB_RISE_END);
			const float forwardT = math::SmoothStep((t - CLIMB_RISE_END) / (1.0f - CLIMB_RISE_END));

			CharacterController& controller = GetController();
			// 登り始めてから動く床に運ばれた分を足す
			const math::Vec2 carried = controller.GetLedgeCarryOffset();
			const math::Vec2 position{
				math::Lerp(m_startPosition.x, m_endPosition.x, forwardT) + carried.x,
				math::Lerp(m_startPosition.y, m_endPosition.y, riseT) + carried.y
			};

			controller.SetPosition(position);
			controller.SetVelocity(math::Vec2::Zero());

			UpdateClimbModel(carried);
		}


		void PlayerClimbUpState::UpdateClimbModel(const math::Vec2& carried) const
		{
			// 見た目：モデルはぶら下がった位置に置いたままにして、アニメーションの中の体の動きで登らせる
			// （当たり判定は Update の計算どおりに動く。登りきったら当たり判定の位置に戻る）
			const PlayerParameter& param = GetParam();
			GetPlayer().SetModelAnchor(m_startPosition + carried);
			GetPlayer().SetModelOffset(math::Vec2(
				param.ledgeHangModelForward + param.climbModelForward,
				param.ledgeHangModelY + param.climbModelY));
		}


		void PlayerClimbUpState::Exit()
		{
			CharacterController& controller = GetController();

			// 登りきった瞬間、モデルを当たり判定の位置に戻す。
			// そのままだと「登りの最後の姿勢（腰が高い）」のまま崖の上に移り、立ちの姿勢への補間中に浮いて見える。
			// なので、今モデルを描いている位置と当たり判定の位置の差を見た目のずらしにして、補間と同じ時間で 0 に戻す
			const PlayerParameter& param = GetParam();
			const float facing = static_cast<float>(GetPlayer().GetFacing());
			const math::Vec2 anchor = m_startPosition + controller.GetLedgeCarryOffset();
			const math::Vec2 drawnPosition{
				anchor.x + (param.ledgeHangModelForward + param.climbModelForward) * facing,
				anchor.y + param.ledgeHangModelY + param.climbModelY
			};
			const math::Vec2 position = controller.GetPosition();
			GetPlayer().ClearModelAnchor();
			GetPlayer().FadeOutModelOffset(
				math::Vec2((drawnPosition.x - position.x) * facing, drawnPosition.y - position.y),
				CharacterBase::DEFAULT_ANIMATION_INTERPOLATE_TIME);

			controller.SetGravityEnabled(true);
			controller.EndLedgeCarry();
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
		// 物をつかんで動かす / 鏡を回す


		void PlayerGrabState::Enter()
		{
			// つかんだ物の方を向く（回すだけのときは向きを変えない：回す向きは今の向きで決まるため）
			Player& player = GetPlayer();
			if (player.IsGrabbing())
			{
				player.SetFacing(player.GetGrabObjectSide());
			}
			UpdateAnimation();
		}


		void PlayerGrabState::Update(const float deltaTime)
		{
			Player& player = GetPlayer();
			if (player.IsGrabbing())
			{
				// 物の方を向いたまま、ゆっくり動く（UpdateFacingFromInput は呼ばない）
				player.SetFacing(player.GetGrabObjectSide());
				m_machine->ApplyGrabMove(deltaTime);
			}
			else
			{
				// 回しているだけのときは、その場で止まる
				m_machine->ApplyStop(deltaTime);
			}
			UpdateAnimation();
		}


		void PlayerGrabState::UpdateAnimation() const
		{
			const Player& player = GetPlayer();
			PlayerAnimation animation = PlayerAnimation::GrabIdle;
			const float velocityX = GetController().GetVelocity().x;
			if (player.IsRotatingObject())
			{
				// 鏡を回す：押す動き
				animation = PlayerAnimation::Push;
			}
			else if (math::Abs(velocityX) >= STOP_SPEED)
			{
				// 向いている方（物の方）へ進んでいれば押す、離れていれば引く
				animation = velocityX * static_cast<float>(player.GetFacing()) > 0.0f ? PlayerAnimation::Push : PlayerAnimation::Pull;
			}
			// 同じアニメーションなら PlayAnimation の中で何もしない
			PlayAnimation(static_cast<int>(animation));
		}


		int PlayerGrabState::CheckTransition()
		{
			if (!GetController().IsOnGround())
			{
				return ToID(PlayerStateID::Fall);
			}
			const Player& player = GetPlayer();
			if (!player.IsGrabbing() && !player.IsRotatingObject())
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
