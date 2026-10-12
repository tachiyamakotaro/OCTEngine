/**
 * @file GameplaySEPlayer.cpp
 * @brief インゲームの SE（プレイヤーの動き・ダメージ・ギミック）を鳴らす
 */
#include "stdafx.h"
#include "GameplaySEPlayer.h"

#include <algorithm>
#include "Source/Actor/Character/CharacterStatus.h"
#include "Source/Actor/Character/CharacterStateMachine.h"
#include "Source/Actor/Character/Player/Player.h"
#include "Source/Actor/Character/Player/PlayerStateMachine.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Sound/SoundManager.h"
#include "Source/Stage/MirrorInteractor.h"
#include "Source/Stage/Stage.h"


namespace
{
	/** まだステートを覚えていない */
	constexpr int NO_STATE = -1;
	/** 扉の開き具合がこれ以上変わったら「動いた」とみなす */
	constexpr float DOOR_MOVE_EPSILON = 0.0001f;
	/** 鏡がこれ以上動いたら「引きずっている」とみなす */
	constexpr float MIRROR_MOVE_EPSILON = 0.01f;


	using app::actor::PlayerStateID;

	/** 地面に立っているステートか（着地の判定に使う） */
	bool IsGroundState(const int id)
	{
		return id == static_cast<int>(PlayerStateID::Idle)
			|| id == static_cast<int>(PlayerStateID::Run)
			|| id == static_cast<int>(PlayerStateID::Crouch);
	}

	/** 空中のステートか */
	bool IsAirState(const int id)
	{
		return id == static_cast<int>(PlayerStateID::Jump)
			|| id == static_cast<int>(PlayerStateID::Fall);
	}

	/** プレイヤーの今のステート（ステートマシンがまだなければ NO_STATE） */
	int GetStateID(const app::actor::Player& player)
	{
		const app::actor::CharacterStateMachine* stateMachine = player.GetStateMachine();
		return stateMachine != nullptr ? stateMachine->GetCurrentStateID() : NO_STATE;
	}

	/** プレイヤーの今のHP（ステータスがまだなければ 0） */
	int GetHp(const app::actor::Player& player)
	{
		const app::actor::CharacterStatus* status = player.GetStatus();
		return status != nullptr ? status->GetHp() : 0;
	}
}


namespace app
{
	GameplaySEPlayer::GameplaySEPlayer()
		: m_prevStateID(NO_STATE)
		, m_prevHp(0)
		, m_airTime(0.0f)
		, m_footstepTimer(0.0f)
		, m_wasRunning(false)
		, m_receiverLit()
		, m_switchPressed()
		, m_lightPlatformSolid()
		, m_indicatorOn()
		, m_doorOpenAmount()
		, m_doorDirection()
		, m_mistCount(0)
		, m_mirrorPlatformCount(0)
		, m_isStageSnapshotPending(false)
		, m_slideHandle(INVALID_SE_HANDLE)
		, m_slideIdleTimer(0.0f)
		, m_playedThisFrame()
		, m_lastPlayedKind(enSoundKind_None)
		, m_playCount(0)
	{}


	GameplaySEPlayer::~GameplaySEPlayer()
	{
		StopLoops();
	}


	void GameplaySEPlayer::Reset(const stage::Stage& stage, const actor::Player& player)
	{
		StopLoops();
		TakePlayerSnapshot(player);
		TakeStageSnapshot(stage);
		// 戻した直後のステージの更新で光を計算し直すので（最初から光が当たっている受光器が点くなど）、
		// 次の Update でもう一度覚え直すだけにする
		m_isStageSnapshotPending = true;
		m_airTime = 0.0f;
		m_footstepTimer = 0.0f;
		m_wasRunning = false;
		m_slideIdleTimer = 0.0f;
	}


	void GameplaySEPlayer::Update(const float deltaTime, const stage::Stage& stage, const actor::Player& player, const stage::MirrorInteractor& mirrorInteractor)
	{
		m_playedThisFrame.clear();

		UpdatePlayer(deltaTime, player);
		UpdateStage(stage);
		UpdateMirror(deltaTime, mirrorInteractor);
	}


	void GameplaySEPlayer::Play(const int kind)
	{
		if (!isEnabled)
		{
			return;
		}
		// 同じフレームに同じ音を何度も鳴らさない（受光器が2つ同時に点いたときなど）
		if (std::find(m_playedThisFrame.begin(), m_playedThisFrame.end(), kind) != m_playedThisFrame.end())
		{
			return;
		}
		m_playedThisFrame.push_back(kind);
		m_lastPlayedKind = kind;
		++m_playCount;
		if (SoundManager::IsCreated())
		{
			SoundManager::Get().PlaySE(kind);
		}
	}


	void GameplaySEPlayer::StopLoops()
	{
		if (m_slideHandle != INVALID_SE_HANDLE && SoundManager::IsCreated())
		{
			SoundManager::Get().StopSE(m_slideHandle);
		}
		m_slideHandle = INVALID_SE_HANDLE;
	}


	bool GameplaySEPlayer::IsSlidePlaying() const
	{
		return SoundManager::IsCreated() && SoundManager::Get().IsPlayingSE(m_slideHandle);
	}


	void GameplaySEPlayer::UpdatePlayer(const float deltaTime, const actor::Player& player)
	{
		const int stateID = GetStateID(player);
		const int hp = GetHp(player);
		if (m_prevStateID == NO_STATE)
		{
			// まだ覚えていなければ、覚えるだけ
			TakePlayerSnapshot(player);
			return;
		}

		const bool isStateChanged = stateID != m_prevStateID;
		if (isStateChanged)
		{
			switch (static_cast<PlayerStateID>(stateID))
			{
			case PlayerStateID::Jump:
				Play(enSoundKind_PlayerJump);
				break;
			case PlayerStateID::HardLand:
				Play(enSoundKind_PlayerHardLand);
				break;
			case PlayerStateID::LedgeHang:
				Play(enSoundKind_PlayerLedgeGrab);
				break;
			case PlayerStateID::ClimbUp:
				Play(enSoundKind_PlayerClimbUp);
				break;
			case PlayerStateID::Dead:
				Play(enSoundKind_PlayerDeath);
				break;
			default:
				// 空中から地面のステートに移った = 着地（少しの段差を降りただけなら鳴らさない）
				if (IsAirState(m_prevStateID) && IsGroundState(stateID) && m_airTime >= minAirTimeForLand)
				{
					Play(enSoundKind_PlayerLand);
				}
				break;
			}
		}

		// ダメージ（HP が減った。0 になったときは死亡の音だけにする）
		if (hp < m_prevHp && hp > 0)
		{
			Play(enSoundKind_PlayerDamage);
		}

		// 空中にいる時間（着地の音を鳴らすか決めるのに使う）
		if (IsAirState(stateID))
		{
			m_airTime += deltaTime;
		}
		else
		{
			m_airTime = 0.0f;
		}

		// 足音：地面を走っている間、一定の間隔で鳴らす
		const actor::CharacterController& controller = player.GetController();
		const bool isRunning = stateID == static_cast<int>(PlayerStateID::Run)
			&& controller.IsOnGround()
			&& math::Abs(controller.GetVelocity().x) >= minFootstepSpeed;
		if (isRunning)
		{
			// 走り始めは少しだけ待ってから1歩目
			if (!m_wasRunning)
			{
				m_footstepTimer = firstFootstepDelay;
			}
			m_footstepTimer -= deltaTime;
			if (m_footstepTimer <= 0.0f)
			{
				Play(enSoundKind_PlayerFootstep);
				// 引ききれなかった分は持ち越す（フレームレートが変わっても同じ間隔になる）。ただし一度に何歩も鳴らさない
				m_footstepTimer = math::Max(m_footstepTimer + math::Max(footstepInterval, 0.05f), 0.0f);
			}
		}
		m_wasRunning = isRunning;

		m_prevStateID = stateID;
		m_prevHp = hp;
	}


	void GameplaySEPlayer::UpdateStage(const stage::Stage& stage)
	{
		// リセットの直後と、エディタでギミックを足したり消したりしたときは、覚え直すだけにする
		if (m_isStageSnapshotPending || !IsSameLayout(stage))
		{
			TakeStageSnapshot(stage);
			m_isStageSnapshotPending = false;
			return;
		}

		// ---- 受光器 ----
		const std::vector<stage::LightReceiver>& receivers = stage.GetLightReceivers();
		for (size_t i = 0; i < receivers.size(); ++i)
		{
			if (receivers[i].isLit != m_receiverLit[i])
			{
				Play(receivers[i].isLit ? enSoundKind_ReceiverOn : enSoundKind_ReceiverOff);
				m_receiverLit[i] = receivers[i].isLit;
			}
		}

		// ---- 光の足場 ----
		const std::vector<stage::LightPlatform>& platforms = stage.GetLightPlatforms();
		for (size_t i = 0; i < platforms.size(); ++i)
		{
			const bool isSolid = platforms[i].IsSolid();
			if (isSolid && !m_lightPlatformSolid[i])
			{
				Play(enSoundKind_LightPlatformOn);
			}
			m_lightPlatformSolid[i] = isSolid;
		}

		// ---- 合図のランプ ----
		const std::vector<stage::Indicator>& indicators = stage.GetIndicators();
		for (size_t i = 0; i < indicators.size(); ++i)
		{
			if (indicators[i].isOn && !m_indicatorOn[i])
			{
				Play(enSoundKind_IndicatorOn);
			}
			m_indicatorOn[i] = indicators[i].isOn;
		}

		// ---- 床スイッチ ----
		const std::vector<stage::FloorSwitch>& switches = stage.GetFloorSwitches();
		for (size_t i = 0; i < switches.size(); ++i)
		{
			if (switches[i].isPressed && !m_switchPressed[i])
			{
				Play(enSoundKind_SwitchOn);
			}
			m_switchPressed[i] = switches[i].isPressed;
		}

		// ---- 扉：動く向きが変わったときに鳴らす（開き始め・閉まり始め）----
		const std::vector<stage::Door>& doors = stage.GetDoors();
		for (size_t i = 0; i < doors.size(); ++i)
		{
			const float delta = doors[i].openAmount - m_doorOpenAmount[i];
			const int direction = delta > DOOR_MOVE_EPSILON ? 1 : (delta < -DOOR_MOVE_EPSILON ? -1 : 0);
			if (direction != 0 && direction != m_doorDirection[i])
			{
				Play(direction > 0 ? enSoundKind_DoorOpen : enSoundKind_DoorClose);
				m_doorDirection[i] = direction;
			}
			m_doorOpenAmount[i] = doors[i].openAmount;
		}

		// ---- 霧 ----
		for (const stage::MistEmitter& mist : stage.GetMistEmitters())
		{
			if (mist.isFiredThisFrame)
			{
				Play(enSoundKind_MistBurst);
			}
		}

		// ---- 鏡の足場（いくつ同時に見えても1回だけ）----
		for (const stage::MirrorPlatform& platform : stage.GetMirrorPlatforms())
		{
			if (platform.isRevealedThisFrame)
			{
				Play(enSoundKind_MirrorPlatformReveal);
				break;
			}
		}
	}


	void GameplaySEPlayer::UpdateMirror(const float deltaTime, const stage::MirrorInteractor& mirrorInteractor)
	{
		if (mirrorInteractor.IsRotatedThisFrame())
		{
			Play(enSoundKind_MirrorRotate);
		}
		if (mirrorInteractor.IsGrabbedThisFrame())
		{
			Play(enSoundKind_MirrorGrab);
		}

		// 引きずる音：鏡が動いている間だけループで鳴らす（止まって少したったら止める）
		const bool isMoving = mirrorInteractor.IsDragging() && mirrorInteractor.GetMovedDistanceThisFrame() > MIRROR_MOVE_EPSILON;
		if (isMoving)
		{
			m_slideIdleTimer = 0.0f;
			if (isEnabled && SoundManager::IsCreated() && !SoundManager::Get().IsPlayingSE(m_slideHandle))
			{
				m_slideHandle = SoundManager::Get().PlaySE(enSoundKind_MirrorSlide, INVALID_VOLUME, true);
			}
			return;
		}
		m_slideIdleTimer += deltaTime;
		if (!mirrorInteractor.IsDragging() || m_slideIdleTimer >= slideStopDelay || !isEnabled)
		{
			StopLoops();
		}
	}


	void GameplaySEPlayer::TakePlayerSnapshot(const actor::Player& player)
	{
		m_prevStateID = GetStateID(player);
		m_prevHp = GetHp(player);
	}


	void GameplaySEPlayer::TakeStageSnapshot(const stage::Stage& stage)
	{
		m_receiverLit.clear();
		for (const stage::LightReceiver& receiver : stage.GetLightReceivers())
		{
			m_receiverLit.push_back(receiver.isLit);
		}
		m_lightPlatformSolid.clear();
		for (const stage::LightPlatform& platform : stage.GetLightPlatforms())
		{
			m_lightPlatformSolid.push_back(platform.IsSolid());
		}
		m_indicatorOn.clear();
		for (const stage::Indicator& indicator : stage.GetIndicators())
		{
			m_indicatorOn.push_back(indicator.isOn);
		}
		m_switchPressed.clear();
		for (const stage::FloorSwitch& floorSwitch : stage.GetFloorSwitches())
		{
			m_switchPressed.push_back(floorSwitch.isPressed);
		}
		m_doorOpenAmount.clear();
		m_doorDirection.clear();
		for (const stage::Door& door : stage.GetDoors())
		{
			m_doorOpenAmount.push_back(door.openAmount);
			m_doorDirection.push_back(0);
		}
		m_mistCount = stage.GetMistEmitters().size();
		m_mirrorPlatformCount = stage.GetMirrorPlatforms().size();
	}


	bool GameplaySEPlayer::IsSameLayout(const stage::Stage& stage) const
	{
		return m_receiverLit.size() == stage.GetLightReceivers().size()
			&& m_lightPlatformSolid.size() == stage.GetLightPlatforms().size()
			&& m_indicatorOn.size() == stage.GetIndicators().size()
			&& m_switchPressed.size() == stage.GetFloorSwitches().size()
			&& m_doorOpenAmount.size() == stage.GetDoors().size()
			&& m_mistCount == stage.GetMistEmitters().size()
			&& m_mirrorPlatformCount == stage.GetMirrorPlatforms().size();
	}
}
