/**
 * @file Player.cpp
 * @brief プレイヤー（探検家）
 */
#include "stdafx.h"
#include "Player.h"
#include "PlayerStateMachine.h"
#include "PlayerStatus.h"


namespace app
{
	namespace actor
	{
		namespace
		{
			/**
			 * @brief アニメーションデータ（PlayerAnimation と並びを合わせる）
			 * @note 専用のアニメーションができるまでは、今あるもので代用している（★仮）
			 *       モデル（Y Bot）とボーンの並びが同じ tka しか使えない（ユニティちゃんの tka は混ぜない）
			 */
			const AnimationData PLAYER_ANIMATION_DATA[] =
			{
				{ "Assets/animData/player/Player_Idle.tka",        true },	// Idle
				{ "Assets/animData/player/Player_Run.tka",         true },	// Run
				{ "Assets/animData/player/Player_Jump.tka",        false },	// Jump
				{ "Assets/animData/player/Player_Jump.tka",        false },	// Fall      ★仮
				{ "Assets/animData/player/Player_Idle.tka",        true },	// Crouch    ★仮
				{ "Assets/animData/player/Player_HangingIdle.tka", true },	// LedgeHang
				{ "Assets/animData/player/Player_Climb.tka",       false },	// ClimbUp
				{ "Assets/animData/player/Player_Idle.tka",        false },	// HardLand  ★仮
				{ "Assets/animData/player/Player_Idle.tka",        false },	// Dead      ★仮
				{ "Assets/animData/player/Player_PullIdle.tka",    true },	// GrabIdle
				{ "Assets/animData/player/Player_Push.tka",        true },	// Push
				{ "Assets/animData/player/Player_PullObject.tka",  true },	// Pull
			};
			static_assert(
				sizeof(PLAYER_ANIMATION_DATA) / sizeof(PLAYER_ANIMATION_DATA[0]) == static_cast<size_t>(PlayerAnimation::Num),
				"PLAYER_ANIMATION_DATA の数が PlayerAnimation と合っていません");

			/** モデルデータ */
			const ModelData PLAYER_MODEL_DATA =
			{
				"Assets/modelData/player/Player_remy.tkm",
				PLAYER_ANIMATION_DATA,
				static_cast<int>(PlayerAnimation::Num),
				enModelUpAxisZ,
			};
		}


		void Player::SetInputDevice(const input::IInputDevice* device)
		{
			m_playerController.SetInputDevice(device);
		}


		void Player::SetMirrored(const bool isMirrored)
		{
			m_playerController.SetMirrorX(isMirrored);
			SetFacing(isMirrored ? -1 : 1);
		}


		bool Player::Start()
		{
			// モデルを先に読み込んでおくと、最初のステートの Enter でアニメーションが再生される
			InitModel(PLAYER_MODEL_DATA);
			return CharacterBase::Start();
		}


		std::unique_ptr<CharacterStatus> Player::CreateStatus()
		{
			return std::make_unique<PlayerStatus>(m_parameter.invincibleTime);
		}


		std::unique_ptr<CharacterStateMachine> Player::CreateStateMachine()
		{
			return std::make_unique<PlayerStateMachine>(this);
		}


		void Player::SetGrabbing(const bool isGrabbing, const float objectOffsetX)
		{
			m_isGrabbing = isGrabbing;
			m_grabObjectOffsetX = objectOffsetX;
		}


		void Player::NotifyObjectRotated()
		{
			m_rotateActionTimer = m_parameter.rotateActionTime;
		}


		void Player::UpdateCommand(const float deltaTime)
		{
			m_playerController.Update(m_command);

			// 鏡を回す動きの残り時間（ステートの更新より前に減らす）
			if (m_rotateActionTimer > 0.0f)
			{
				m_rotateActionTimer -= deltaTime;
			}
		}


		Player::Player()
			: m_parameter()
			, m_playerController(nullptr)
			, m_isGrabbing(false)
			, m_grabObjectOffsetX(0.0f)
			, m_rotateActionTimer(0.0f)
		{}
	}
}
