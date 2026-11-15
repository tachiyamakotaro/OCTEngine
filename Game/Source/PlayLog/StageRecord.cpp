/**
 * @file StageRecord.cpp
 * @brief 1ステージぶんのプレイの記録
 * @details ログの文字は UTF-8 で書きたいので u8"..." を使う（ふつうの "..." だと Shift-JIS になるため）。
 */
#include "stdafx.h"
#include "StageRecord.h"
#include <cmath>


namespace app
{
	namespace playlog
	{
		namespace
		{
			/** 死んだ理由の表示名 */
			const char* ToText(const DeathCause cause)
			{
				switch (cause)
				{
				case DeathCause::Pit:		return u8"奈落";
				case DeathCause::Damage:	return u8"落下ダメージ";
				}
				return u8"不明";
			}
		}


		StageRecord::StageRecord(const std::string& stageName)
			: m_stageName(stageName)
			, m_deaths()
			, m_timeUpCount(0)
			, m_timeUpTotal()
			, m_isCleared(false)
			, m_clearTime()
		{}


		bool StageRecord::IsStage(const std::string& stageName) const
		{
			return m_stageName == stageName;
		}


		void StageRecord::AddDeath(const DeathCause cause, const math::Vec2& position)
		{
			m_deaths.push_back({ cause, position });
		}


		void StageRecord::AddTimeUp(const PlayTime& playedTime)
		{
			++m_timeUpCount;
			m_timeUpTotal += playedTime;
		}


		void StageRecord::Clear(const PlayTime& clearTime)
		{
			// デバッグでクリアし直したときは、最後のクリアタイムにする
			m_isCleared = true;
			m_clearTime = clearTime;
		}


		PlayTime StageRecord::GetTotalTime() const
		{
			return m_timeUpTotal + m_clearTime;
		}


		void StageRecord::WriteTo(std::ostream& out) const
		{
			out << u8"[" << m_stageName << u8"] " << (m_isCleared ? u8"クリア" : u8"未クリア") << u8"\n";
			if (m_isCleared)
			{
				out << u8"  クリアタイム : " << m_clearTime.ToText() << u8"（最後の挑戦）\n";
			}
			out << u8"  かかった時間 : " << GetTotalTime().ToText() << u8"（時間切れでやり直したぶんも含む）\n";
			out << u8"  死亡回数     : " << GetDeathCount()
				<< u8"（奈落 " << CountDeaths(DeathCause::Pit)
				<< u8" / 落下ダメージ " << CountDeaths(DeathCause::Damage) << u8"）\n";
			out << u8"  時間切れ     : " << m_timeUpCount << u8"\n";

			// どこで死んだか（ステージの調整用）
			for (std::size_t i = 0; i < m_deaths.size(); ++i)
			{
				const DeathRecord& death = m_deaths[i];
				out << u8"    " << (i + 1) << u8"回目 : " << ToText(death.cause)
					<< u8" (x " << static_cast<int>(std::round(death.position.x))
					<< u8", y " << static_cast<int>(std::round(death.position.y)) << u8")\n";
			}
		}


		int StageRecord::CountDeaths(const DeathCause cause) const
		{
			int count = 0;
			for (const DeathRecord& death : m_deaths)
			{
				if (death.cause == cause)
				{
					++count;
				}
			}
			return count;
		}
	}
}
