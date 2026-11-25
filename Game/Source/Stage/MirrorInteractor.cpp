/**
 * @file MirrorInteractor.cpp
 * @brief プレイヤーによる反射鏡の操作（回転・移動）
 */
#include "stdafx.h"
#include "MirrorInteractor.h"
#include "Stage.h"
#include "Source/Actor/Character/CharacterBase.h"


namespace
{
	/** RB で回す段数（右＝時計回り。LightMirror::Rotate は正で反時計回り） */
	constexpr int ROTATE_RIGHT_STEPS = -1;
	/** LB で回す段数（左＝反時計回り） */
	constexpr int ROTATE_LEFT_STEPS = 1;
}


namespace app
{
	namespace stage
	{
		void MirrorInteractor::Update(Stage& stage, const actor::CharacterBase& character)
		{
			GimmickManager& gimmicks = stage.GetGimmicks();
			const actor::CharacterCommand& command = character.GetCommand();
			const math::Vec2& position = character.GetPosition();
			ClearFrameEvents();

			// つかむ（B）を離したら、またつかめるようにする
			if (!command.grabHold)
			{
				m_isWaitingGrabRelease = false;
			}

			// 引きずり中：つかむを離した・地面から離れた・鏡が消えた ならやめる
			if (m_isDragging)
			{
				const bool isValid = dynamic_cast<LightMirror*>(gimmicks.Get(static_cast<size_t>(m_focusedIndex))) != nullptr;
				if (!isValid || !command.grabHold || !character.GetController().IsOnGround())
				{
					m_isDragging = false;
				}
			}

			if (!m_isDragging)
			{
				m_focusedIndex = FindNearestMirror(stage, character);
			}
			if (m_focusedIndex < 0)
			{
				return;
			}

			LightMirror& mirror = static_cast<LightMirror&>(*gimmicks.Get(static_cast<size_t>(m_focusedIndex)));

			// 回転（RB で右＝時計回り、LB で左＝反時計回り。プレイヤーの向きには関係しない）
			const RotateDirection direction = ToRotateDirection(command);
			if (direction != RotateDirection::None && mirror.isRotatable)
			{
				mirror.Rotate(direction == RotateDirection::Right ? ROTATE_RIGHT_STEPS : ROTATE_LEFT_STEPS);
				m_rotatedDirection = direction;
			}

			// 引きずり開始
			if (!m_isDragging && !m_isWaitingGrabRelease && command.grabHold && mirror.isMovable && character.GetController().IsOnGround())
			{
				m_isDragging = true;
				m_isGrabbedThisFrame = true;
				m_dragOffsetX = mirror.center.x - position.x;
			}

			// 引きずり中：キャラクターについていく（地形にぶつかる位置には動かさない）
			if (m_isDragging)
			{
				const float targetX = mirror.ClampToRail(position.x + m_dragOffsetX);
				LightMirror moved = mirror;
				moved.center.x = targetX;
				if (!stage.OverlapsTerrain(moved.GetBounds()))
				{
					m_movedDistanceThisFrame = math::Abs(targetX - mirror.center.x);
					mirror.center.x = targetX;
				}

				// レールの端・地形で鏡がついてこられず、キャラクターが離れたら手を離す
				// （ふつうの歩き方に戻る。B を押したままなら、一度離すまでつかみ直さない）
				if (IsLeftBehind(mirror, position.x))
				{
					m_isDragging = false;
					m_isWaitingGrabRelease = true;
				}
			}
		}


		void MirrorInteractor::ClearFrameEvents()
		{
			m_rotatedDirection = RotateDirection::None;
			m_isGrabbedThisFrame = false;
			m_movedDistanceThisFrame = 0.0f;
		}


		void MirrorInteractor::Reset()
		{
			m_focusedIndex = -1;
			m_isDragging = false;
			m_dragOffsetX = 0.0f;
			m_rotatedDirection = RotateDirection::None;
			m_isGrabbedThisFrame = false;
			m_movedDistanceThisFrame = 0.0f;
			m_isWaitingGrabRelease = false;
		}


		int MirrorInteractor::FindNearestMirror(const Stage& stage, const actor::CharacterBase& character) const
		{
			const GimmickManager& gimmicks = stage.GetGimmicks();
			const math::Vec2& position = character.GetPosition();

			int nearestIndex = -1;
			float nearestDistance = interactRangeX;
			for (const LightMirror& mirror : gimmicks.View<LightMirror>())
			{
				if (!mirror.isRotatable && !mirror.isMovable)
				{
					continue;
				}
				const float dx = math::Abs(mirror.center.x - position.x);
				const float dy = mirror.center.y - position.y;
				if (dx <= nearestDistance && dy >= -interactRangeY * 0.25f && dy <= interactRangeY)
				{
					nearestDistance = dx;
					nearestIndex = gimmicks.FindIndex(&mirror);
				}
			}
			return nearestIndex;
		}


		MirrorInteractor::RotateDirection MirrorInteractor::ToRotateDirection(const actor::CharacterCommand& command)
		{
			// 両方同時に押したときは、どちらに回すか決められないので回さない
			if (command.rotateLeftTrigger == command.rotateRightTrigger)
			{
				return RotateDirection::None;
			}
			return command.rotateRightTrigger ? RotateDirection::Right : RotateDirection::Left;
		}


		bool MirrorInteractor::IsLeftBehind(const LightMirror& mirror, const float characterX) const
		{
			// つかんだときの「鏡とキャラクターの差」から、今の差がどれだけずれたか
			const float offsetX = mirror.center.x - characterX;
			return math::Abs(offsetX - m_dragOffsetX) > releaseDistance;
		}


		MirrorInteractor::MirrorInteractor()
			: m_focusedIndex(-1)
			, m_isDragging(false)
			, m_dragOffsetX(0.0f)
			, m_rotatedDirection(RotateDirection::None)
			, m_isGrabbedThisFrame(false)
			, m_movedDistanceThisFrame(0.0f)
			, m_isWaitingGrabRelease(false)
		{}
	}
}
