/**
 * @file PlayLog.cpp
 * @brief プレイログ
 * @details ログの文字は UTF-8 で書きたいので u8"..." を使う（ふつうの "..." だと Shift-JIS になるため）。
 */
#include "stdafx.h"
#include "PlayLog.h"
#include "nlohmann/json.hpp"
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

			/** 日時 → 続きの記録に書く数（1970年からの秒） */
			long long ToSaveTime(const std::tm& time)
			{
				std::tm copy = time;
				return static_cast<long long>(std::mktime(&copy));
			}

			/** 続きの記録に書いた数 → 日時 */
			std::tm FromSaveTime(const long long saved)
			{
				const std::time_t value = static_cast<std::time_t>(saved);
				std::tm localTime = {};
				localtime_s(&localTime, &value);
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
			, m_filePath()
			, m_resumeTimes()
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
				// 最初に書くときに場所を決める（続きから遊んだプレイは、前と同じ場所に書き直す）
				if (m_filePath.empty())
				{
					m_filePath = m_file.MakeNewPath(m_startTime);
				}
				m_file.Save(m_filePath, text.str());
				// ゲームクリアしたら続きはない。途中なら、続きから遊べるように中身を残す
				if (m_isGameCleared)
				{
					m_file.EraseContinueData();
				}
				else
				{
					m_file.SaveContinueData(ToContinueText());
				}
			}
			// 次のプレイのために空にする
			m_isPlaying = false;
			m_stages.clear();
			m_currentStageIndex = -1;
			m_isGameCleared = false;
			m_filePath.clear();
			m_resumeTimes.clear();
		}


		bool PlayLog::ContinuePlay()
		{
			if (m_isPlaying)
			{
				return false;
			}
			const std::optional<std::string> text = m_file.LoadContinueData();
			if (!text.has_value() || !RestoreFromContinueText(*text))
			{
				return false;
			}
			m_isPlaying = true;
			m_resumeTimes.push_back(GetLocalNow());
			return true;
		}


		void PlayLog::DiscardContinue()
		{
			m_file.EraseContinueData();
		}


		void PlayLog::StartPlay()
		{
			m_isPlaying = true;
			m_stages.clear();
			m_currentStageIndex = -1;
			m_isGameCleared = false;
			m_startTime = GetLocalNow();
			m_filePath.clear();
			m_resumeTimes.clear();
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
			for (const std::tm& resumeTime : m_resumeTimes)
			{
				out << u8"続きから再開   : " << ToDateText(resumeTime) << u8"\n";
			}
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


		std::string PlayLog::ToContinueText() const
		{
			nlohmann::ordered_json root;
			root["filePath"] = m_filePath;
			root["startTime"] = ToSaveTime(m_startTime);
			nlohmann::ordered_json resumeTimes = nlohmann::ordered_json::array();
			for (const std::tm& resumeTime : m_resumeTimes)
			{
				resumeTimes.push_back(ToSaveTime(resumeTime));
			}
			root["resumeTimes"] = std::move(resumeTimes);
			nlohmann::ordered_json stages = nlohmann::ordered_json::array();
			for (const StageRecord& stage : m_stages)
			{
				stages.push_back(stage.ToJson());
			}
			root["stages"] = std::move(stages);
			// ステージの名前が UTF-8 でなくても止まらないように、読めない文字は置きかえて書く
			return root.dump(2, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
		}


		bool PlayLog::RestoreFromContinueText(const std::string& text)
		{
			// 壊れたファイルでも止まらないように、例外を出さずに読む
			const nlohmann::ordered_json root = nlohmann::ordered_json::parse(text, nullptr, false);
			if (root.is_discarded() || !root.is_object()
				|| !root.contains("startTime") || !root["startTime"].is_number_integer()
				|| !root.contains("stages") || !root["stages"].is_array())
			{
				return false;
			}
			m_stages.clear();
			for (const nlohmann::ordered_json& stage : root["stages"])
			{
				m_stages.push_back(StageRecord::FromJson(stage));
			}
			m_currentStageIndex = -1;
			m_isGameCleared = false;
			m_startTime = FromSaveTime(root["startTime"].get<long long>());
			m_filePath = root.contains("filePath") && root["filePath"].is_string() ? root["filePath"].get<std::string>() : std::string();
			m_resumeTimes.clear();
			if (root.contains("resumeTimes") && root["resumeTimes"].is_array())
			{
				for (const nlohmann::ordered_json& resumeTime : root["resumeTimes"])
				{
					if (resumeTime.is_number_integer())
					{
						m_resumeTimes.push_back(FromSaveTime(resumeTime.get<long long>()));
					}
				}
			}
			return true;
		}
	}
}
