#pragma once
#include "Source/Title/TitleParameter.h"


/**
 * @file EntranceTimeline.h
 * @brief タイトル画面の演出の時間の流れ（何秒に何が起きるか）
 */
namespace app
{
	namespace title
	{
		/**
		 * @brief タイトル画面の演出の時間の流れ
		 * @details ・始まると、プレイヤーが走ってきて門の少し手前で止まって待つ
		 *          ・BeginEnter（遊ぶステージを決めたとき）で、門の前まで走る → 門のほうを向く → 扉が開く
		 *            → 門の中へ走って入る が始まる
		 *            走ってくる途中で決めたら、手前で止まってから始める
		 *          ・毎フレーム Update で時間を進め、そのフレームに起きたこと（扉が開き始めた など）を返す。
		 *            扉を開けて音を鳴らすのは TitleDirector
		 *          ・どこまで進んだかを 0〜1 の割合で答える（GetRunRate など）。
		 *            プレイヤーを動かすのは TitlePlayerModel（このクラスは時間を数えるだけ）
		 *          ・ロゴはメニュー（ui::TitleMenu）が自分の時間で出す（このクラスは使わない）
		 */
		class EntranceTimeline
		{
		public:
			/** そのフレームに起きたこと */
			struct Events
			{
				bool isDoorOpened = false;	//!< 扉が開き始めた
			};

		public:
			EntranceTimeline() = default;
			~EntranceTimeline() = default;

			void Init(const EntranceTiming& timing = EntranceTiming());
			/** 時間を進める（そのフレームに起きたことを返す） */
			Events Update(float deltaTime);
			/** 門へ入り始める（遊ぶステージを決めたとき。2回目からは何もしない） */
			void BeginEnter();

			/** 走ってきた割合（0 = 走り始めの位置 〜 1 = 門の少し手前。止まる前はゆっくりになる） */
			float GetRunRate() const;
			/** 門の前まで走った割合（0 = 門の少し手前 〜 1 = 門の前。入り始める前は 0） */
			float GetApproachRate() const;
			/** 門の中へ入った割合（0 = 門の前 〜 1 = 扉の奥の光の中。走り出しはゆっくり、だんだん速くなる） */
			float GetWalkInRate() const;
			/** 走っているか（走ってくる間・門の前まで走る間・門の中へ入っていく間） */
			bool IsRunning() const;
			/** 門のほうを向いた割合（0 = 右向き 〜 1 = 門のほう） */
			float GetTurnRate() const;
			/** 門の中へ入るときに、まっすぐ奥を向いた割合（0 = 門のほう（少し横顔） 〜 1 = まっすぐ奥） */
			float GetWalkInTurnRate() const;
			/** 入る演出が終わったか（次のシーンへ移ってよいか） */
			bool IsEnterFinished() const;
			/** 扉が開ききるまでの時間 */
			float GetDoorOpenTime() const { return m_timing.doorOpenTime; }

		private:
			/** 門へ入り始めた時間からの時間（入り始めていなければ負の値） */
			float GetEnterTime() const;
			/** start から duration かけて 0 → 1（範囲の外は 0 か 1） */
			static float RateBetween(float time, float start, float duration);
			/** 前のフレームから今のフレームの間に、その時間を通ったか */
			static bool IsPassed(float previous, float current, float time) { return previous < time && time <= current; }

		private:
			EntranceTiming m_timing;
			/** シーンが始まってからの時間 */
			float m_time = 0.0f;
			/** 門へ入り始めた時間（シーンが始まってから。走ってくる途中で押したら、止まる時間） */
			float m_enterStart = 0.0f;
			/** 門へ入り始めたか */
			bool m_isEntering = false;
		};
	}
}
