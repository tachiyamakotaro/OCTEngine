/**
 * @file EventCamera.cpp
 * @brief イベントカメラ（特定の場面でだけカメラを寄せる）
 */
#include "stdafx.h"
#include "EventCamera.h"

#include "GameCamera.h"
#include "Source/Core/Math/MathUtil.h"
#include <cfloat>


namespace
{
	/** 時間が 0 のときに割り算しないための、いちばん短い時間 */
	constexpr float MIN_BLEND_TIME = 0.001f;
}


namespace app
{
	namespace camera
	{
		EventCamera::EventCamera()
			: m_parameter()
			, m_event()
			, m_elapsed(0.0f)
			, m_outStartTime(0.0f)
			, m_isPlaying(false)
			, m_shot()
		{}


		void EventCamera::Init(const EventCameraParameter& parameter)
		{
			m_parameter = parameter;
			Reset();
		}


		void EventCamera::Play(const CameraEvent& event)
		{
			const EventShotSetting& setting = GetSetting(event.GetKind());
			if (!setting.isEnabled)
			{
				return;
			}
			// 今の演出のほうが大事なら入れ替えない（ゴールの演出中にギミックで寄り直さない）
			if (m_isPlaying && event.GetPriority() < m_event.GetPriority())
			{
				return;
			}

			// 寄っている途中・寄ったままの途中で入れ替えたときは、今の効き具合から寄り始める（カメラが一度戻らないように）
			const float currentWeight = m_isPlaying ? m_shot.weight : 0.0f;
			const float blendIn = math::Max(setting.blendInTime, 0.0f);
			m_event = event;
			m_elapsed = blendIn * currentWeight;
			// Release を待つ場面は、呼ばれるまで戻り始めない
			m_outStartTime = event.IsHeldUntilRelease() ? FLT_MAX : blendIn + math::Max(setting.holdTime, 0.0f);
			m_isPlaying = true;
			// blendIn が 0 の場面（ステージ開始）は、この時点で寄った見え方にしておく（すぐ SnapTo しても寄っているように）
			m_shot.weight = CalcWeight(setting);
		}


		void EventCamera::Release()
		{
			if (!m_isPlaying || m_elapsed >= m_outStartTime)
			{
				return;
			}
			// 今の効き具合から戻り始める（寄りきる前に呼ばれても、カメラが跳ねないように）
			const float blendOut = math::Max(GetSetting(m_event.GetKind()).blendOutTime, MIN_BLEND_TIME);
			m_outStartTime = m_elapsed - blendOut * (1.0f - m_shot.weight);
		}


		void EventCamera::Reset()
		{
			m_isPlaying = false;
			m_elapsed = 0.0f;
			m_outStartTime = 0.0f;
			m_shot.weight = 0.0f;
		}


		void EventCamera::Update(const float deltaTime, const math::Rect& playerBody)
		{
			if (!m_isPlaying)
			{
				m_shot.weight = 0.0f;
				return;
			}

			const EventShotSetting& setting = GetSetting(m_event.GetKind());
			m_elapsed += deltaTime;
			// 戻りきったら終わり（ImGui でこの場面を OFF にしたときもすぐ終わる）
			if (!setting.isEnabled || m_elapsed >= m_outStartTime + math::Max(setting.blendOutTime, 0.0f))
			{
				Reset();
				return;
			}

			m_shot.area = m_event.MakeFocusArea(playerBody, setting.isIncludeTarget).Expanded(setting.margin);
			m_shot.keepArea = playerBody.Expanded(setting.margin);
			m_shot.zoom = setting.zoom;
			m_shot.weight = CalcWeight(setting);
		}


		bool EventCamera::IsLockingPlayer() const
		{
			return m_isPlaying && GetSetting(m_event.GetKind()).isLockPlayer && m_elapsed < m_outStartTime;
		}


		void EventCamera::ApplyTo(GameCamera& camera) const
		{
			camera.SetEventShot(m_shot);
		}


		const EventShotSetting& EventCamera::GetSetting(const CameraEventKind kind) const
		{
			switch (kind)
			{
			case CameraEventKind::Goal:
				return m_parameter.goal;
			case CameraEventKind::StageStart:
				return m_parameter.stageStart;
			case CameraEventKind::Death:
				return m_parameter.death;
			case CameraEventKind::GimmickSolved:
			default:
				return m_parameter.gimmickSolved;
			}
		}


		float EventCamera::CalcWeight(const EventShotSetting& setting) const
		{
			// ふだんに戻る（1 → 0）
			if (m_elapsed >= m_outStartTime)
			{
				const float blendOut = math::Max(setting.blendOutTime, MIN_BLEND_TIME);
				return math::SmoothStep(1.0f - (m_elapsed - m_outStartTime) / blendOut);
			}
			// 寄る（0 → 1。blendIn が 0 なら最初から 1）
			const float blendIn = setting.blendInTime;
			if (blendIn > 0.0f && m_elapsed < blendIn)
			{
				return math::SmoothStep(m_elapsed / blendIn);
			}
			// 寄ったまま（1）
			return 1.0f;
		}
	}
}
