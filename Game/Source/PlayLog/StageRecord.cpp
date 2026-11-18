/**
 * @file StageRecord.cpp
 * @brief 1ステージぶんのプレイの記録
 * @details ログの文字は UTF-8 で書きたいので u8"..." を使う（ふつうの "..." だと Shift-JIS になるため）。
 */
#include "stdafx.h"
#include "StageRecord.h"
#include "nlohmann/json.hpp"
#include <cmath>
#include <type_traits>


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

			/** 死んだ理由を、続きの記録に書く名前にする（ログの表示とは別。日本語にしない） */
			const char* ToSaveName(const DeathCause cause)
			{
				return cause == DeathCause::Damage ? "Damage" : "Pit";
			}

			/** 続きの記録の名前から、死んだ理由に戻す */
			DeathCause FromSaveName(const std::string& name)
			{
				return name == "Damage" ? DeathCause::Damage : DeathCause::Pit;
			}

			/** JSON から数を読む（なければ fallback） */
			template <typename T>
			T ReadValue(const nlohmann::ordered_json& node, const char* key, const T fallback)
			{
				if (!node.is_object() || !node.contains(key))
				{
					return fallback;
				}
				const nlohmann::ordered_json& value = node[key];
				if constexpr (std::is_same_v<T, bool>)
				{
					return value.is_boolean() ? value.get<bool>() : fallback;
				}
				else
				{
					return value.is_number() ? value.get<T>() : fallback;
				}
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


		nlohmann::ordered_json StageRecord::ToJson() const
		{
			nlohmann::ordered_json node;
			node["stageName"] = m_stageName;
			node["isCleared"] = m_isCleared;
			node["clearTime"] = m_clearTime.ToSeconds();
			node["timeUpCount"] = m_timeUpCount;
			node["timeUpTotal"] = m_timeUpTotal.ToSeconds();
			nlohmann::ordered_json deaths = nlohmann::ordered_json::array();
			for (const DeathRecord& death : m_deaths)
			{
				deaths.push_back({ { "cause", ToSaveName(death.cause) }, { "x", death.position.x }, { "y", death.position.y } });
			}
			node["deaths"] = std::move(deaths);
			return node;
		}


		StageRecord StageRecord::FromJson(const nlohmann::ordered_json& node)
		{
			const std::string name = node.contains("stageName") && node["stageName"].is_string()
				? node["stageName"].get<std::string>() : std::string();
			StageRecord record(name);
			record.m_isCleared = ReadValue(node, "isCleared", false);
			record.m_clearTime = PlayTime(ReadValue(node, "clearTime", 0.0f));
			record.m_timeUpCount = ReadValue(node, "timeUpCount", 0);
			record.m_timeUpTotal = PlayTime(ReadValue(node, "timeUpTotal", 0.0f));
			if (node.contains("deaths") && node["deaths"].is_array())
			{
				for (const nlohmann::ordered_json& death : node["deaths"])
				{
					const std::string cause = death.contains("cause") && death["cause"].is_string()
						? death["cause"].get<std::string>() : std::string();
					const math::Vec2 position(ReadValue(death, "x", 0.0f), ReadValue(death, "y", 0.0f));
					record.m_deaths.push_back({ FromSaveName(cause), position });
				}
			}
			return record;
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
