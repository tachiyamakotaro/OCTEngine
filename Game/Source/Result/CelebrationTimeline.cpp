/**
 * @file CelebrationTimeline.cpp
 * @brief 宝物の間の演出の時間の流れ
 */
#include "stdafx.h"
#include "CelebrationTimeline.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace result
	{
		void CelebrationTimeline::Init(const CelebrationTiming& timing)
		{
			m_timing = timing;
			m_time = 0.0f;
		}


		CelebrationTimeline::Events CelebrationTimeline::Update(const float deltaTime)
		{
			const float previous = m_time;
			m_time += deltaTime;

			Events events;
			events.isLidOpened = IsPassed(previous, m_timing.lidOpenStart);
			events.isGemShown = IsPassed(previous, m_timing.gemRiseStart);
			events.isLogoShown = IsPassed(previous, m_timing.logoStart);
			for (int i = 0; i < m_timing.hopCount; ++i)
			{
				events.isHopped = events.isHopped || IsPassed(previous, m_timing.hopStart + m_timing.hopInterval * static_cast<float>(i));
			}
			return events;
		}


		void CelebrationTimeline::SkipToEnd()
		{
			m_time = math::Max(m_time, m_timing.promptStart);
		}


		float CelebrationTimeline::GetIrisCloseRate() const
		{
			return 1.0f - math::SmoothStep(RateBetween(m_timing.irisOpenStart, m_timing.irisOpenTime));
		}


		float CelebrationTimeline::GetRunRate() const
		{
			// 最後のほうでゆっくりになって止まる（2乗のイーズアウト）
			const float t = RateBetween(m_timing.runStart, m_timing.runEnd - m_timing.runStart);
			return 1.0f - (1.0f - t) * (1.0f - t) * 0.35f - (1.0f - t) * 0.65f;
		}


		bool CelebrationTimeline::IsRunning() const
		{
			return m_time >= m_timing.runStart && m_time < m_timing.runEnd;
		}


		int CelebrationTimeline::GetHopIndex() const
		{
			for (int i = 0; i < m_timing.hopCount; ++i)
			{
				const float start = m_timing.hopStart + m_timing.hopInterval * static_cast<float>(i);
				if (m_time >= start && m_time < start + m_timing.hopTime)
				{
					return i;
				}
			}
			return -1;
		}


		float CelebrationTimeline::GetHopRate() const
		{
			const int index = GetHopIndex();
			if (index < 0)
			{
				return 0.0f;
			}
			return RateBetween(m_timing.hopStart + m_timing.hopInterval * static_cast<float>(index), m_timing.hopTime);
		}


		float CelebrationTimeline::GetCelebrateRate() const
		{
			return math::SmoothStep(RateBetween(m_timing.hopStart, m_timing.GetHopEnd() - m_timing.hopStart));
		}


		float CelebrationTimeline::GetLidOpenRate() const
		{
			// 勢いよく開いて、最後はゆっくり止まる
			const float t = RateBetween(m_timing.lidOpenStart, m_timing.lidOpenTime);
			return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
		}


		float CelebrationTimeline::GetGemRiseRate() const
		{
			return math::SmoothStep(RateBetween(m_timing.gemRiseStart, m_timing.gemRiseTime));
		}


		float CelebrationTimeline::GetLogoRate() const
		{
			return RateBetween(m_timing.logoStart, m_timing.logoTime);
		}


		float CelebrationTimeline::GetPromptTime() const
		{
			return math::Max(0.0f, m_time - m_timing.promptStart);
		}


		float CelebrationTimeline::GetDollyRate() const
		{
			return math::SmoothStep(RateBetween(0.0f, m_timing.dollyTime));
		}


		float CelebrationTimeline::RateBetween(const float start, const float duration) const
		{
			if (duration <= 0.0f)
			{
				return m_time >= start ? 1.0f : 0.0f;
			}
			return math::Clamp01((m_time - start) / duration);
		}
	}
}
