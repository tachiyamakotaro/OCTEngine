/**
 * @file EntranceTimeline.cpp
 * @brief タイトル画面の演出の時間の流れ
 */
#include "stdafx.h"
#include "EntranceTimeline.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace title
	{
		namespace
		{
			/** 入り始めていないときの、入り始めてからの時間（どの時間にも届かない値） */
			constexpr float NOT_ENTERED_TIME = -1.0f;
		}


		void EntranceTimeline::Init(const EntranceTiming& timing)
		{
			m_timing = timing;
			m_time = 0.0f;
			m_enterStart = 0.0f;
			m_isEntering = false;
		}


		EntranceTimeline::Events EntranceTimeline::Update(const float deltaTime)
		{
			const float previous = GetEnterTime();
			m_time += deltaTime;
			const float current = GetEnterTime();

			Events events;
			events.isDoorOpened = m_isEntering && IsPassed(previous, current, m_timing.doorOpenDelay);
			return events;
		}


		void EntranceTimeline::BeginEnter()
		{
			if (m_isEntering)
			{
				return;
			}
			// 走ってくる途中なら、手前で止まってから門へ向かう
			m_enterStart = math::Max(m_time, m_timing.runEnd);
			m_isEntering = true;
		}


		float EntranceTimeline::GetRunRate() const
		{
			// 最後のほうでゆっくりになって止まる（宝物の間の CelebrationTimeline::GetRunRate と同じ形）
			const float t = RateBetween(m_time, m_timing.runStart, m_timing.runEnd - m_timing.runStart);
			return 1.0f - (1.0f - t) * (1.0f - t) * 0.35f - (1.0f - t) * 0.65f;
		}


		float EntranceTimeline::GetApproachRate() const
		{
			// 待っている所から走り出して、門の前でゆっくり止まる
			return math::SmoothStep(RateBetween(GetEnterTime(), 0.0f, m_timing.approachTime));
		}


		float EntranceTimeline::GetWalkInRate() const
		{
			// 止まっている所から走り出して、だんだん速くなる（光の中に消えるまで止まらない）
			const float t = RateBetween(GetEnterTime(), m_timing.walkInDelay, m_timing.walkInTime);
			return t * t * 0.4f + t * 0.6f;
		}


		bool EntranceTimeline::IsRunning() const
		{
			const bool isRunningIn = m_time >= m_timing.runStart && m_time < m_timing.runEnd;
			const float enterTime = GetEnterTime();
			const bool isApproaching = enterTime >= 0.0f && enterTime < m_timing.approachTime;
			// 門の中へ入り始めたら、光の中に消えるまで（消えたあとも）走ったまま
			const bool isWalkingIn = enterTime >= m_timing.walkInDelay;
			return isRunningIn || isApproaching || isWalkingIn;
		}


		float EntranceTimeline::GetTurnRate() const
		{
			return math::SmoothStep(RateBetween(GetEnterTime(), m_timing.approachTime, m_timing.turnTime));
		}


		float EntranceTimeline::GetWalkInTurnRate() const
		{
			return math::SmoothStep(RateBetween(GetEnterTime(), m_timing.walkInDelay, m_timing.walkInTurnTime));
		}


		bool EntranceTimeline::IsEnterFinished() const
		{
			return GetEnterTime() >= m_timing.leaveDelay;
		}


		float EntranceTimeline::GetEnterTime() const
		{
			if (!m_isEntering)
			{
				return NOT_ENTERED_TIME;
			}
			return m_time - m_enterStart;
		}


		float EntranceTimeline::RateBetween(const float time, const float start, const float duration)
		{
			if (duration <= 0.0f)
			{
				return time >= start ? 1.0f : 0.0f;
			}
			return math::Clamp01((time - start) / duration);
		}
	}
}
