/**
 * @file PlayLog.cpp
 * @brief プレイログ
 * @details ログの文字は UTF-8 で書きたいので u8"..." を使う（ふつうの "..." だと Shift-JIS になるため）。
 */
#include "stdafx.h"
#include "PlayLog.h"
#include <sstream>


namespace app
{
	namespace playlog
	{
		PlayLog* PlayLog::m_instance = nullptr;


		namespace
		{
			/**
			 * @brief ファイルに書き出すか
			 * @details 提出用の ROM などでファイルを作りたくないときは false にする（記録はするが、書き出さない）
			 */
			constexpr bool IS_SAVE_ENABLED = true;

			/** 今の日時（この PC の時刻） */
			std::tm GetLocalNow()
			{
				const std::time_t now = std::time(nullptr);
				std::tm localTime = {};
				localtime_s(&localTime, &now);
				return localTime;
			}

			/** 日時の表示用の文字列（例：2026/11/15 18:59:30） */
			std::string ToDateText(const std::tm& time)
			{
				char text[32] = {};
				std::strftime(text, sizeof(text), "%Y/%m/%d %H:%M:%S", &time);
				return text;
			}
		}


		PlayLog::PlayLog()
			: m_isPlaying(false)
			, m_stages()
			, m_currentStageIndex(-1)
			, m_startTime()
			, m_isGameCleared(false)
			, m_file()
		{}


		void PlayLog::StartStage(const std::string& stageName)
		{
			// インゲームに入った最初のステージなら、新しいプレイを始める
			if (!m_isPlaying)
			{
				StartPlay();
			}
			// 前に遊んだステージなら、その記録の続きにする（デバッグでステージを切り替えて戻ってきたとき）
			for (std::size_t i = 0; i < m_stages.size(); ++i)
			{
				if (m_stages[i].IsStage(stageName))
				{
					m_currentStageIndex = static_cast<int>(i);
					return;
				}
			}
			m_stages.emplace_back(stageName);
			m_currentStageIndex = static_cast<int>(m_stages.size()) - 1;
		}


		void PlayLog::RecordDeath(const DeathCause cause, const math::Vec2& position)
		{
			StageRecord* stage = GetCurrentStage();
			if (stage == nullptr)
			{
				return;
			}
			stage->AddDeath(cause, position);
		}


		void PlayLog::RecordTimeUp(const PlayTime& playedTime)
		{
			StageRecord* stage = GetCurrentStage();
			if (stage == nullptr)
			{
				return;
			}
			stage->AddTimeUp(playedTime);
		}


		void PlayLog::RecordClear(const PlayTime& clearTime)
		{
			StageRecord* stage = GetCurrentStage();
			if (stage == nullptr)
			{
				return;
			}
			stage->Clear(clearTime);
		}


		void PlayLog::RecordGameClear()
		{
			m_isGameCleared = true;
		}


		void PlayLog::FinishPlay()
		{
			if (!m_isPlaying)
			{
				return;
			}
			if (IS_SAVE_ENABLED)
			{
				std::ostringstream text;
				WriteTo(text, GetLocalNow());
				m_file.SaveAsNewFile(m_startTime, text.str());
			}
			// 次のプレイのために空にする
			m_isPlaying = false;
			m_stages.clear();
			m_currentStageIndex = -1;
			m_isGameCleared = false;
		}


		void PlayLog::StartPlay()
		{
			m_isPlaying = true;
			m_stages.clear();
			m_currentStageIndex = -1;
			m_isGameCleared = false;
			m_startTime = GetLocalNow();
		}


		StageRecord* PlayLog::GetCurrentStage()
		{
			if (m_currentStageIndex < 0 || m_currentStageIndex >= static_cast<int>(m_stages.size()))
			{
				return nullptr;
			}
			return &m_stages[m_currentStageIndex];
		}


		void PlayLog::WriteTo(std::ostream& out, const std::tm& endTime) const
		{
			out << u8"==== Ray Seeker プレイログ ====\n";
			out << u8"遊び始めた日時 : " << ToDateText(m_startTime) << u8"\n";
			out << u8"遊び終えた日時 : " << ToDateText(endTime) << u8"\n";
			out << u8"結果           : " << (m_isGameCleared ? u8"ゲームクリア" : u8"途中で終了（タイトルへ戻った・ゲームを閉じた など）") << u8"\n";
			out << u8"\n";

			// ステージごと（遊んだ順）。合計もここで数える
			PlayTime totalTime;
			int totalDeaths = 0;
			int totalTimeUps = 0;
			for (const StageRecord& stage : m_stages)
			{
				stage.WriteTo(out);
				out << u8"\n";
				totalTime += stage.GetTotalTime();
				totalDeaths += stage.GetDeathCount();
				totalTimeUps += stage.GetTimeUpCount();
			}

			out << u8"---- 合計 ----\n";
			out << u8"合計時間     : " << totalTime.ToText() << u8"（各ステージのかかった時間の合計）\n";
			out << u8"合計死亡回数 : " << totalDeaths << u8"\n";
			out << u8"合計時間切れ : " << totalTimeUps << u8"\n";
		}
	}
}
