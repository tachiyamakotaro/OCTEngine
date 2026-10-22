/**
 * @file ShadeInteractor.cpp
 * @brief プレイヤーによる日よけ板の操作（つかんで左右に動かす）
 */
#include "stdafx.h"
#include "ShadeInteractor.h"
#include "Stage.h"
#include "Source/Actor/Character/CharacterBase.h"


namespace app
{
	namespace stage
	{
		void ShadeInteractor::Update(Stage& stage, const actor::CharacterBase& character, const bool canGrab)
		{
			GimmickManager& gimmicks = stage.GetGimmicks();
			const actor::CharacterCommand& command = character.GetCommand();
			const math::Vec2& position = character.GetPosition();
			const bool isOnGround = character.GetController().IsOnGround();
			m_isGrabbedThisFrame = false;
			m_movedDistanceThisFrame = 0.0f;

			// 引きずり中：つかむを離した・地面から離れた・板が消えた ならやめる
			if (m_isDragging)
			{
				const bool isValid = dynamic_cast<ShadeBoard*>(gimmicks.Get(static_cast<size_t>(m_focusedIndex))) != nullptr;
				if (!isValid || !command.grabHold || !isOnGround)
				{
					m_isDragging = false;
				}
			}

			if (!m_isDragging)
			{
				m_focusedIndex = canGrab ? FindNearestBoard(stage, character) : -1;
			}
			if (m_focusedIndex < 0)
			{
				return;
			}

			ShadeBoard& board = static_cast<ShadeBoard&>(*gimmicks.Get(static_cast<size_t>(m_focusedIndex)));

			// 引きずり開始
			if (!m_isDragging && command.grabHold && board.isMovable && isOnGround)
			{
				m_isDragging = true;
				m_isGrabbedThisFrame = true;
				m_dragOffsetX = board.GetCenterX() - position.x;
			}

			// 引きずり中：キャラクターについていく（地形にぶつかる位置には動かさない）
			if (m_isDragging)
			{
				const float targetX = board.ClampToRail(position.x + m_dragOffsetX);
				ShadeBoard moved = board;
				moved.SetCenterX(targetX);
				if (!stage.OverlapsTerrain(moved.rect))
				{
					m_movedDistanceThisFrame = math::Abs(targetX - board.GetCenterX());
					board.SetCenterX(targetX);
				}
			}
		}


		void ShadeInteractor::Reset()
		{
			m_focusedIndex = -1;
			m_isDragging = false;
			m_dragOffsetX = 0.0f;
			m_isGrabbedThisFrame = false;
			m_movedDistanceThisFrame = 0.0f;
		}


		int ShadeInteractor::FindNearestBoard(const Stage& stage, const actor::CharacterBase& character) const
		{
			const GimmickManager& gimmicks = stage.GetGimmicks();
			const math::Vec2& position = character.GetPosition();

			int nearestIndex = -1;
			float nearestDistance = interactRangeX;
			for (const ShadeBoard& board : gimmicks.View<ShadeBoard>())
			{
				if (!board.isMovable)
				{
					continue;
				}
				// 横：体の中心が板の幅の中なら 0、外なら近いほうの端までの距離
				float dx = 0.0f;
				if (position.x < board.rect.min.x) { dx = board.rect.min.x - position.x; }
				else if (position.x > board.rect.max.x) { dx = position.x - board.rect.max.x; }
				// 縦：板の下の面が足元から interactRangeY まで、板の上の面が足元より上
				const bool isInRangeY = board.rect.min.y - position.y <= interactRangeY && board.rect.max.y > position.y;
				if (dx <= nearestDistance && isInRangeY)
				{
					nearestDistance = dx;
					nearestIndex = gimmicks.FindIndex(&board);
				}
			}
			return nearestIndex;
		}


		ShadeInteractor::ShadeInteractor()
			: m_focusedIndex(-1)
			, m_isDragging(false)
			, m_dragOffsetX(0.0f)
			, m_isGrabbedThisFrame(false)
			, m_movedDistanceThisFrame(0.0f)
		{}
	}
}
