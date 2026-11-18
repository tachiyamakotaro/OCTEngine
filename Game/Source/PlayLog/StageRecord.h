/**
 * @file StageRecord.h
 * @brief 1ステージぶんのプレイの記録（死亡・時間切れ・クリアタイム）
 * @details 1ステージの記録のデータと、そのデータを変えるルール・ログへの書き方を、このクラスにまとめている（高凝集）。
 *          外からは「死んだ」「時間切れになった」「クリアした」と命じるだけで、回数の数え方や時間の足し方は知らなくてよい。
 */
#pragma once
#include "Source/Core/Math/Vec2.h"
#include "Source/PlayLog/PlayTime.h"
#include "nlohmann/json_fwd.hpp"
#include <ostream>
#include <string>
#include <vector>


namespace app
{
	namespace playlog
	{
		/**
		 * @brief 死んだ理由
		 */
		enum class DeathCause
		{
			Pit,	//!< 奈落に落ちた（画面の下に落ちた）
			Damage,	//!< 高い所から落ちた・落下ダメージで HP がなくなった
		};


		/**
		 * @brief 1ステージぶんのプレイの記録
		 */
		class StageRecord
		{
		public:
			/** @param stageName ステージの名前（StageCatalog.h の表の name） */
			explicit StageRecord(const std::string& stageName);

			/** このステージの記録か */
			bool IsStage(const std::string& stageName) const;

			/**
			 * @brief 死んだことを記録する
			 * @param cause 死んだ理由
			 * @param position 死んだ位置（足元。どこで死にやすいかを見るため）
			 */
			void AddDeath(DeathCause cause, const math::Vec2& position);
			/**
			 * @brief 時間切れになったことを記録する
			 * @param playedTime 時間切れまでに遊んだ時間（やり直したぶんの時間として、かかった時間に足す）
			 */
			void AddTimeUp(const PlayTime& playedTime);
			/**
			 * @brief クリアしたことを記録する
			 * @param clearTime クリアタイム（最後の挑戦の時間。時間切れでやり直す前の時間は入れない）
			 */
			void Clear(const PlayTime& clearTime);

			/** 死んだ回数 */
			int GetDeathCount() const { return static_cast<int>(m_deaths.size()); }
			/** 時間切れになった回数 */
			int GetTimeUpCount() const { return m_timeUpCount; }
			/** このステージにかかった時間（時間切れでやり直したぶん + クリアタイム） */
			PlayTime GetTotalTime() const;

			/** ログに書く（ステージ1つぶんの段落） */
			void WriteTo(std::ostream& out) const;

			/** 続きから遊ぶための記録にする（記録の中身を全部 JSON にする） */
			nlohmann::ordered_json ToJson() const;
			/**
			 * @brief 続きから遊ぶための記録から戻す
			 * @details 足りない項目は 0（死亡なし・未クリア）として読む
			 */
			static StageRecord FromJson(const nlohmann::ordered_json& node);


		private:
			/**
			 * @brief 1回ぶんの死亡
			 */
			struct DeathRecord
			{
				DeathCause cause;
				math::Vec2 position;
			};


		private:
			/** 死んだ理由ごとの回数 */
			int CountDeaths(DeathCause cause) const;


		private:
			/** ステージの名前 */
			std::string m_stageName;
			/** 死んだ記録（死んだ順） */
			std::vector<DeathRecord> m_deaths;
			/** 時間切れになった回数 */
			int m_timeUpCount;
			/** 時間切れでやり直したぶんの時間の合計 */
			PlayTime m_timeUpTotal;
			/** クリアしたか */
			bool m_isCleared;
			/** クリアタイム（最後の挑戦） */
			PlayTime m_clearTime;
		};
	}
}
