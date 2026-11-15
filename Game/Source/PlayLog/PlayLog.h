/**
 * @file PlayLog.h
 * @brief プレイログ（どのステージで何回死んだか・クリアタイム・合計時間をファイルに残す）
 * @details 1プレイ＝インゲームに入ってから、インゲーム以外のシーン（タイトル・リザルトなど）に移るまで。
 *          1プレイにつき1ファイル作る（ファイルは何個でも増えていく）。
 *
 *          使い方（呼ぶ側は「〜した」と伝えるだけ。数え方・書き出し方は知らなくてよい）
 *          ・StartStage(名前)         … ステージを始めたとき（プレイ中でなければ、ここで新しいプレイを始める）
 *          ・RecordDeath(理由, 位置)  … 死んだとき（1回の死亡につき1回だけ呼ぶ）
 *          ・RecordTimeUp(遊んだ時間) … 時間切れになったとき
 *          ・RecordClear(クリアタイム)… ゴールしたとき
 *          ・RecordGameClear()        … 最後のステージをクリアしたとき
 *          ・FinishPlay()             … インゲーム以外のシーンに移ったとき・ゲームを終えたとき（ここでファイルを作る）
 *
 *          インゲームのシーン（DebugScene）はステージごとに作り直されるので、記録はシーンではなくここに持つ。
 *          SoundManager と同じく Game::Start で作り、Game のデストラクタで消す。
 *          どこに書くかは PlayLogFile、1ステージぶんの記録は StageRecord が持つ。
 */
#pragma once
#include "Source/Core/Math/Vec2.h"
#include "Source/PlayLog/PlayLogFile.h"
#include "Source/PlayLog/PlayTime.h"
#include "Source/PlayLog/StageRecord.h"
#include <ctime>
#include <ostream>
#include <string>
#include <vector>


namespace app
{
	namespace playlog
	{
		/**
		 * @brief プレイログ
		 */
		class PlayLog
		{
		public:
			/**
			 * @brief ステージを始めた（同じステージに戻ってきたときは、前の記録の続きにする）
			 * @details プレイ中でなければ、ここで新しいプレイを始める
			 * @param stageName ステージの名前（StageCatalog.h の表の name）
			 */
			void StartStage(const std::string& stageName);
			/** 今のステージで死んだ */
			void RecordDeath(DeathCause cause, const math::Vec2& position);
			/** 今のステージで時間切れになった（playedTime：時間切れまでに遊んだ時間） */
			void RecordTimeUp(const PlayTime& playedTime);
			/** 今のステージをクリアした */
			void RecordClear(const PlayTime& clearTime);
			/** 最後のステージまでクリアした */
			void RecordGameClear();
			/**
			 * @brief プレイを終える（1プレイぶんを新しいファイルに書き出して、記録を空にする）
			 * @details プレイ中でなければ何もしない（リザルト → タイトルのときなど）
			 */
			void FinishPlay();


		public:
			/** インスタンスを作成（Game::Start で呼ぶ） */
			static void CreateInstance()
			{
				if (m_instance == nullptr)
				{
					m_instance = new PlayLog();
				}
			}
			/** インスタンスを取得 */
			static PlayLog& Get() { return *m_instance; }
			/** 作られているか */
			static bool IsCreated() { return m_instance != nullptr; }
			/** インスタンスを削除 */
			static void DestroyInstance()
			{
				delete m_instance;
				m_instance = nullptr;
			}


		private:
			PlayLog();
			~PlayLog() = default;
			PlayLog(const PlayLog&) = delete;
			PlayLog& operator=(const PlayLog&) = delete;

			/** 新しいプレイを始める（記録を空にして、遊び始めた日時を覚える） */
			void StartPlay();
			/** 今のステージの記録（StartStage する前なら nullptr） */
			StageRecord* GetCurrentStage();
			/** ログ全体を書く（見出し・ステージごとの記録・合計） */
			void WriteTo(std::ostream& out, const std::tm& endTime) const;


		private:
			/** プレイ中か（インゲームに入ってから、ほかのシーンに移るまで） */
			bool m_isPlaying;
			/** ステージごとの記録（遊んだ順） */
			std::vector<StageRecord> m_stages;
			/** 今のステージの記録の番号（m_stages の番号。まだなければ -1） */
			int m_currentStageIndex;
			/** 遊び始めた日時 */
			std::tm m_startTime;
			/** 最後のステージまでクリアしたか */
			bool m_isGameCleared;
			/** 書き出すファイル */
			PlayLogFile m_file;

			/** シングルトンインスタンス */
			static PlayLog* m_instance;
		};
	}
}
