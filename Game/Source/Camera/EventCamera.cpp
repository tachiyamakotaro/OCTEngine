/**
 * @file EventCamera.cpp
 * @brief イベントカメラ（特定の場面でだけカメラを寄せる）
 */
#include "stdafx.h"
#include "EventCamera.h"

#include "GameCamera.h"
#include "Source/Core/Math/MathUtil.h"


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
			if (!GetSetting(event.GetKind()).isEnabled)
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
			m_event = event;
			m_elapsed = math::Max(m_parameter.blendInTime, MIN_BLEND_TIME) * currentWeight;
			m_isPlaying = true;
		}


		void EventCamera::Reset()
		{
			m_isPlaying = false;
			m_elapsed = 0.0f;
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
			if (!setting.isEnabled || m_elapsed >= CalcTotalTime(setting))
			{
				Reset();
				return;
			}

			m_shot.area = m_event.MakeFocusArea(playerBody, setting.isIncludeTarget).Expanded(setting.margin);
			m_shot.keepArea = playerBody.Expanded(setting.margin);
			m_shot.zoom = setting.zoom;
			m_shot.weight = CalcWeight(setting);
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
			case CameraEventKind::GimmickSolved:
			default:
				return m_parameter.gimmickSolved;
			}
		}


		float EventCamera::CalcTotalTime(const EventShotSetting& setting) const
		{
			return math::Max(m_parameter.blendInTime, 0.0f) + math::Max(setting.holdTime, 0.0f) + math::Max(m_parameter.blendOutTime, 0.0f);
		}


		float EventCamera::CalcWeight(const EventShotSetting& setting) const
		{
			const float blendIn = math::Max(m_parameter.blendInTime, MIN_BLEND_TIME);
			const float blendOut = math::Max(m_parameter.blendOutTime, MIN_BLEND_TIME);
			const float hold = math::Max(setting.holdTime, 0.0f);

			// 寄る（0 → 1）
			if (m_elapsed < blendIn)
			{
				return math::SmoothStep(m_elapsed / blendIn);
			}
			// 寄ったまま（1）
			const float outStart = blendIn + hold;
			if (m_elapsed < outStart)
			{
				return 1.0f;
			}
			// ふだんに戻る（1 → 0）
			return math::SmoothStep(1.0f - (m_elapsed - outStart) / blendOut);
		}
	}
}
