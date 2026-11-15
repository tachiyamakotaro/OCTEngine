/**
 * @file PlayTime.cpp
 * @brief プレイ時間（秒）を表す値
 */
#include "stdafx.h"
#include "PlayTime.h"
#include <cstdio>


namespace app
{
	namespace playlog
	{
		namespace
		{
			/** 1分の秒数 */
			constexpr float SECONDS_PER_MINUTE = 60.0f;
		}


		PlayTime::PlayTime(const float seconds)
			: m_seconds(seconds > 0.0f ? seconds : 0.0f)
		{}


		PlayTime PlayTime::operator+(const PlayTime& other) const
		{
			return PlayTime(m_seconds + other.m_seconds);
		}


		PlayTime& PlayTime::operator+=(const PlayTime& other)
		{
			m_seconds += other.m_seconds;
			return *this;
		}


		std::string PlayTime::ToText() const
		{
			const int minutes = static_cast<int>(m_seconds / SECONDS_PER_MINUTE);
			const float seconds = m_seconds - static_cast<float>(minutes) * SECONDS_PER_MINUTE;
			char text[32] = {};
			// 秒は 2 桁にそろえる（1:05.30 のように）
			std::snprintf(text, sizeof(text), "%d:%05.2f", minutes, seconds);
			return text;
		}
	}
}
