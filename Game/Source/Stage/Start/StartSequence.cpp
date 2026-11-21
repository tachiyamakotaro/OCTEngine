/**
 * @file StartSequence.cpp
 * @brief ステージ開始の演出の流れ
 */
#include "stdafx.h"
#include "StartSequence.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace stage
	{
		namespace
		{
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


		void StartSequence::Init(const StartTiming& timing)
		{
			m_timing = timing;
			m_initialTiming = timing;
			Reset();
		}


		void StartSequence::Start()
		{
			m_elapsed = 0.0f;
			m_isPlaying = true;
			m_hasOpenStarted = false;
			m_isOpenStartedThisFrame = false;
		}


		void StartSequence::Reset()
		{
			m_elapsed = 0.0f;
			m_isPlaying = false;
			m_hasOpenStarted = false;
			m_isOpenStartedThisFrame = false;
		}


		void StartSequence::Update(const float deltaTime)
		{
			m_isOpenStartedThisFrame = false;
			// 始まっていない・画面の切り替え中（0 が渡される）は進めない
			if (!m_isPlaying || deltaTime <= 0.0f)
			{
				return;
			}
			m_elapsed += deltaTime;

			// 待つ時間を越えた最初のフレームが「開き始めた」合図
			const float openStart = math::Max(m_timing.holdTime, 0.0f);
			if (!m_hasOpenStarted && m_elapsed >= openStart)
			{
				m_hasOpenStarted = true;
				m_isOpenStartedThisFrame = true;
				return;
			}

			// 丸が開ききったら終わり（開き始めた合図のフレームでは終わらせない。合図を必ず1回出すため）
			if (m_hasOpenStarted && m_elapsed >= openStart + math::Max(m_timing.irisOpenTime, 0.0f))
			{
				m_isPlaying = false;
			}
		}


		bool StartSequence::IsHolding() const
		{
			return m_isPlaying && !m_hasOpenStarted;
		}


		float StartSequence::GetIrisCloseRate() const
		{
			if (!m_isPlaying)
			{
				return 0.0f;
			}
			// 小さい丸（startCloseRate）から、ゆっくり始まってゆっくり止まるように開く（0）
			const float open = math::SmoothStep(Progress(m_elapsed, math::Max(m_timing.holdTime, 0.0f), m_timing.irisOpenTime));
			return math::Clamp01(m_timing.startCloseRate) * (1.0f - open);
		}
	}
}
