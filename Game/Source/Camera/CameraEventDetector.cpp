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
		{}


		void CameraEventDetector::Reset(const stage::Stage& stage)
		{
			// 今もう開いている扉は「開いたことがある」にしておく（リスタートの直後に寄らないように）
			m_hasDoorOpened.clear();
			for (const stage::Door& door : stage.GetGimmicks().View<stage::Door>())
			{
				m_hasDoorOpened.push_back(door.openAmount > DOOR_OPENED_AMOUNT);
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
						eventCamera.Play(CameraEvent::GimmickSolved(door.rect));
					}
					m_hasDoorOpened[i] = true;
				}
				++i;
			}
		}


		void CameraEventDetector::DetectIcePassable(const stage::Stage& stage, EventCamera& eventCamera) const
		{
			for (const stage::IceBlock& ice : stage.GetGimmicks().View<stage::IceBlock>())
			{
				// ステージエディタで「Camera Focus」にした氷だけ寄る。守る氷（足場の氷）が溶けたのは失敗なので寄らない
				if (ice.isCameraFocus && ice.isPassableThisFrame && ice.kind == stage::IceKind::Melt)
				{
					eventCamera.Play(CameraEvent::GimmickSolved(ice.rect));
				}
			}
		}


		bool CameraEventDetector::IsSameLayout(const stage::Stage& stage) const
		{
			return m_hasDoorOpened.size() == stage.GetGimmicks().View<stage::Door>().size();
		}
	}
}
