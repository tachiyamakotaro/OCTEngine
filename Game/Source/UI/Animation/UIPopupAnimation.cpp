/**
 * @file UIPopupAnimation.cpp
 * @brief 拡大縮小で出し入れするアニメーション
 */
#include "stdafx.h"
#include "Source/UI/Animation/UIPopupAnimation.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace ui
	{
		namespace
		{
			/** 開く・閉じるのにかかる時間（秒） */
			constexpr float OPEN_TIME = 0.22f;
			constexpr float CLOSE_TIME = 0.14f;
			/** 開き始めの拡大率（ここから少し行きすぎて 1 になる） */
			constexpr float OPEN_START_SCALE = 0.8f;
			/** 閉じ終わりの拡大率 */
			constexpr float CLOSE_END_SCALE = 0.85f;
			/** 開くとき、濃さが 1 になるまでの割合（大きさより早く濃くする） */
			constexpr float OPEN_FADE_RATE = 0.6f;
			/** 行きすぎる強さ（EaseOutBack。1.70158 で約 10% 行きすぎる） */
			constexpr float OVERSHOOT = 1.70158f;

			/** 最初は速く、少し行きすぎてから 1 に戻る */
			float EaseOutBack(const float t)
			{
				const float u = t - 1.0f;
				return 1.0f + (OVERSHOOT + 1.0f) * u * u * u + OVERSHOOT * u * u;
			}

			/** 最初はゆっくり、だんだん速く */
			float EaseIn(const float t)
			{
				return t * t;
			}
		}


		UIPopupAnimation::UIPopupAnimation()
			: m_state(State::Closed)
			, m_rate(0.0f)
			, m_closeFromScale(1.0f)
			, m_closeFromAlpha(1.0f)
		{}


		void UIPopupAnimation::Open()
		{
			if (m_state == State::Opening || m_state == State::Opened)
			{
				return;
			}
			// 閉じている途中なら、閉じた分だけ進んだ所から開く（急に小さくならないように）
			m_rate = m_state == State::Closing ? 1.0f - m_rate : 0.0f;
			m_state = State::Opening;
		}


		void UIPopupAnimation::Close()
		{
			if (m_state == State::Closing || m_state == State::Closed)
			{
				return;
			}
			// 今の大きさ・濃さから小さくする
			m_closeFromScale = GetScale();
			m_closeFromAlpha = GetAlpha();
			m_rate = 0.0f;
			m_state = State::Closing;
		}


		void UIPopupAnimation::Hide()
		{
			m_state = State::Closed;
			m_rate = 0.0f;
		}


		void UIPopupAnimation::Update()
		{
			const float deltaTime = g_gameTime->GetFrameDeltaTime();
			if (m_state == State::Opening)
			{
				m_rate = math::Min(m_rate + deltaTime / OPEN_TIME, 1.0f);
				if (m_rate >= 1.0f)
				{
					m_state = State::Opened;
				}
			}
			else if (m_state == State::Closing)
			{
				m_rate = math::Min(m_rate + deltaTime / CLOSE_TIME, 1.0f);
				if (m_rate >= 1.0f)
				{
					Hide();
				}
			}
		}


		UIGroupTransform UIPopupAnimation::MakeTransform(const Vector2& pivot) const
		{
			UIGroupTransform group;
			group.pivot = pivot;
			group.scale = GetScale();
			group.alpha = GetAlpha();
			return group;
		}


		float UIPopupAnimation::GetScale() const
		{
			switch (m_state)
			{
			case State::Opening:
				return math::Lerp(OPEN_START_SCALE, 1.0f, EaseOutBack(m_rate));
			case State::Closing:
				return math::Lerp(m_closeFromScale, CLOSE_END_SCALE, EaseIn(m_rate));
			case State::Opened:
				return 1.0f;
			case State::Closed:
			default:
				return OPEN_START_SCALE;
			}
		}


		float UIPopupAnimation::GetAlpha() const
		{
			switch (m_state)
			{
			case State::Opening:
				return math::SmoothStep(m_rate / OPEN_FADE_RATE);
			case State::Closing:
				return m_closeFromAlpha * (1.0f - m_rate);
			case State::Opened:
				return 1.0f;
			case State::Closed:
			default:
				return 0.0f;
			}
		}
	}
}
