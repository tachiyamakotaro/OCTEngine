/**
 * @file ClearSequence.cpp
 * @brief クリアの演出の流れ
 */
#include "stdafx.h"
#include "ClearSequence.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/** ゆっくり始まって、ゆっくり止まる（0〜1 → 0〜1） */
			float EaseInOut(const float t)
			{
				return math::SmoothStep(t);
			}

			/** 時間の割合（length が 0 なら、もう終わっている扱い） */
			float Progress(const float elapsed, const float start, const float length)
			{
				if (length <= 0.0f)
				{
					return elapsed >= start ? 1.0f : 0.0f;
				}
				return math::Clamp01((elapsed - start) / length);
			}
		}


		void ClearSequence::Init(const ClearTiming& timing)
		{
			m_timing = timing;
			m_initialTiming = timing;
			Reset();
		}


		void ClearSequence::Start(const stage::Goal& goal)
		{
			m_goal = &goal;
			m_elapsed = 0.0f;
			m_isStarted = true;
		}


		void ClearSequence::Start()
		{
			// 扉の時間を飛ばして、丸が閉じるところから
			m_goal = nullptr;
			m_elapsed = m_timing.GetIrisStartTime();
			m_isStarted = true;
		}


		void ClearSequence::Reset()
		{
			m_goal = nullptr;
			m_elapsed = 0.0f;
			m_isStarted = false;
		}


		void ClearSequence::Update(const float deltaTime)
		{
			if (!m_isStarted)
			{
				return;
			}
			m_elapsed += deltaTime;
		}


		bool ClearSequence::IsFinished() const
		{
			return m_isStarted && m_elapsed >= m_timing.GetTotalTime();
		}


		float ClearSequence::GetDoorOpenRate(const stage::Goal& goal) const
		{
			if (!m_isStarted || m_goal != &goal)
			{
				return 0.0f;
			}
			return EaseInOut(Progress(m_elapsed, 0.0f, m_timing.doorOpenTime));
		}


		float ClearSequence::GetIrisCloseRate() const
		{
			if (!m_isStarted)
			{
				return 0.0f;
			}
			return EaseInOut(Progress(m_elapsed, m_timing.GetIrisStartTime(), m_timing.irisCloseTime));
		}
	}
}
