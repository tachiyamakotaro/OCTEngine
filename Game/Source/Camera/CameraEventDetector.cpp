/**
 * @file CameraEventDetector.cpp
 * @brief ステージのギミックの変化から、カメラが寄る場面を見つける
 */
#include "stdafx.h"
#include "CameraEventDetector.h"

#include "CameraEvent.h"
#include "EventCamera.h"
#include "Source/Stage/Stage.h"


namespace
{
	/** これより開いたら「開いた」とみなす（開き具合 0〜1） */
	constexpr float DOOR_OPENED_AMOUNT = 0.01f;
}


namespace app
{
	namespace camera
	{
		CameraEventDetector::CameraEventDetector()
			: m_hasDoorOpened()
			, m_hasBlockStarted()
			, m_focusedGimmick(nullptr)
		{}


		void CameraEventDetector::Reset(const stage::Stage& stage)
		{
			m_focusedGimmick = nullptr;
			// 今もう開いている扉は「開いたことがある」にしておく（リスタートの直後に寄らないように）
			m_hasDoorOpened.clear();
			for (const stage::Door& door : stage.GetGimmicks().View<stage::Door>())
			{
				m_hasDoorOpened.push_back(door.openAmount > DOOR_OPENED_AMOUNT);
			}
			// 今もう動いている床も「動いたことがある」にしておく
			m_hasBlockStarted.clear();
			for (const stage::MovingBlock& block : stage.GetGimmicks().View<stage::MovingBlock>())
			{
				m_hasBlockStarted.push_back(block.isRunning);
			}
		}


		void CameraEventDetector::Update(const stage::Stage& stage, EventCamera& eventCamera)
		{
			// エディタでギミックを足したり消したりしたときは、覚え直すだけにする
			if (!IsSameLayout(stage))
			{
				Reset(stage);
				return;
			}

			DetectDoorOpened(stage, eventCamera);
			DetectIcePassable(stage, eventCamera);
			DetectBlockStarted(stage, eventCamera);
		}


		void CameraEventDetector::DetectDoorOpened(const stage::Stage& stage, EventCamera& eventCamera)
		{
			size_t i = 0;
			for (const stage::Door& door : stage.GetGimmicks().View<stage::Door>())
			{
				if (!m_hasDoorOpened[i] && door.openAmount > DOOR_OPENED_AMOUNT)
				{
					// ステージエディタで「Camera Focus」にした扉だけ寄る（閉じているときの範囲を映す）
					if (door.isCameraFocus)
					{
						Focus(door, door.rect, eventCamera);
					}
					m_hasDoorOpened[i] = true;
				}
				++i;
			}
		}


		void CameraEventDetector::DetectIcePassable(const stage::Stage& stage, EventCamera& eventCamera)
		{
			for (const stage::IceBlock& ice : stage.GetGimmicks().View<stage::IceBlock>())
			{
				// ステージエディタで「Camera Focus」にした氷だけ寄る。守る氷（足場の氷）が溶けたのは失敗なので寄らない
				if (ice.isCameraFocus && ice.isPassableThisFrame && ice.kind == stage::IceKind::Melt)
				{
					Focus(ice, ice.rect, eventCamera);
				}
			}
		}


		void CameraEventDetector::DetectBlockStarted(const stage::Stage& stage, EventCamera& eventCamera)
		{
			size_t i = 0;
			for (const stage::MovingBlock& block : stage.GetGimmicks().View<stage::MovingBlock>())
			{
				if (!m_hasBlockStarted[i] && block.isRunning)
				{
					// ステージエディタで「Camera Focus」にした床だけ寄る（動く道のり全体を映す）
					if (block.isCameraFocus)
					{
						Focus(block, block.GetPathArea(), eventCamera);
					}
					m_hasBlockStarted[i] = true;
				}
				++i;
			}
		}


		void CameraEventDetector::Focus(const stage::GimmickBase& gimmick, const math::Rect& area, EventCamera& eventCamera)
		{
			eventCamera.Play(CameraEvent::GimmickSolved(area));
			m_focusedGimmick = &gimmick;
		}


		bool CameraEventDetector::IsSameLayout(const stage::Stage& stage) const
		{
			const bool isSameDoors = m_hasDoorOpened.size() == stage.GetGimmicks().View<stage::Door>().size();
			const bool isSameBlocks = m_hasBlockStarted.size() == stage.GetGimmicks().View<stage::MovingBlock>().size();
			return isSameDoors && isSameBlocks;
		}
	}
}
