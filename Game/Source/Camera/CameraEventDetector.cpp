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
	/** 暗がりの中の鏡をそろえたときに映す、暗がりの真ん中の範囲の大きさ */
	constexpr app::math::Vec2 DARK_ZONE_FOCUS_SIZE(400.0f, 300.0f);

	/** 扉が開き始めたか（信号がそろって待っている間も「開き始めた」とみなす） */
	bool HasDoorStartedOpening(const app::stage::Door& door)
	{
		return door.isOpenRequested || door.openAmount > DOOR_OPENED_AMOUNT;
	}
}


namespace app
{
	namespace camera
	{
		CameraEventDetector::CameraEventDetector()
			: m_hasDoorOpened()
			, m_hasBlockStarted()
			, m_hasMirrorAligned()
			, m_focusedGimmick(nullptr)
		{}


		void CameraEventDetector::Reset(const stage::Stage& stage)
		{
			m_focusedGimmick = nullptr;
			// 今もう開いている扉は「開いたことがある」にしておく（リスタートの直後に寄らないように）
			m_hasDoorOpened.clear();
			for (const stage::Door& door : stage.GetGimmicks().View<stage::Door>())
			{
				m_hasDoorOpened.push_back(HasDoorStartedOpening(door));
			}
			// 今もう動いている床も「動いたことがある」にしておく
			m_hasBlockStarted.clear();
			for (const stage::MovingBlock& block : stage.GetGimmicks().View<stage::MovingBlock>())
			{
				m_hasBlockStarted.push_back(block.isRunning);
			}
			// 今もう正しい向きで光を反射している鏡も「そろえたことがある」にしておく
			m_hasMirrorAligned.clear();
			for (const stage::LightMirror& mirror : stage.GetGimmicks().View<stage::LightMirror>())
			{
				m_hasMirrorAligned.push_back(mirror.isCameraFocus && mirror.IsReflectingAtFocusAngle(stage.GetLightResult()));
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
			DetectMirrorAligned(stage, eventCamera);
		}


		void CameraEventDetector::DetectDoorOpened(const stage::Stage& stage, EventCamera& eventCamera)
		{
			size_t i = 0;
			for (const stage::Door& door : stage.GetGimmicks().View<stage::Door>())
			{
				// 扉が動き出す前（openDelay で待っている間）に寄り始める。寄りきってから扉が上がるのを見せる
				if (!m_hasDoorOpened[i] && HasDoorStartedOpening(door))
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


		void CameraEventDetector::DetectMirrorAligned(const stage::Stage& stage, EventCamera& eventCamera)
		{
			const stage::LightTraceResult& light = stage.GetLightResult();
			size_t i = 0;
			for (const stage::LightMirror& mirror : stage.GetGimmicks().View<stage::LightMirror>())
			{
				// ステージエディタで「Camera Focus」にした鏡だけ、正しい向きで初めて光を反射したときに寄る（光の行き先まで映す）
				if (!m_hasMirrorAligned[i] && mirror.isCameraFocus && mirror.IsReflectingAtFocusAngle(light))
				{
					Focus(mirror, GetMirrorFocusArea(stage, mirror), eventCamera);
					m_hasMirrorAligned[i] = true;
				}
				++i;
			}
		}


		math::Rect CameraEventDetector::GetMirrorFocusArea(const stage::Stage& stage, const stage::LightMirror& mirror) const
		{
			// 暗がりの中の鏡は、暗がりの真ん中あたりを映す（鏡のまわりは暗くて、寄っても見えにくいため）
			for (const stage::DarkZone& zone : stage.GetGimmicks().View<stage::DarkZone>())
			{
				if (!zone.isWholeStage && zone.Contains(mirror.center))
				{
					return math::Rect::FromCenter(zone.rect.Center(), DARK_ZONE_FOCUS_SIZE);
				}
			}
			// 暗がりの外の鏡は、鏡と反射した光の行き先を映す
			return mirror.GetFocusArea(stage.GetLightResult());
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
			const bool isSameMirrors = m_hasMirrorAligned.size() == stage.GetGimmicks().View<stage::LightMirror>().size();
			return isSameDoors && isSameBlocks && isSameMirrors;
		}
	}
}
