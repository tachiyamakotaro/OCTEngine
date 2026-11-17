#pragma once
#include "Source/Stage/Goal/GoalParameter.h"


/**
 * @file ClearSequence.h
 * @brief クリアの演出の流れ（扉が開く → 丸が閉じる → 次のステージへ）
 */
namespace app
{
	namespace stage
	{
		class Goal;


		/**
		 * @brief クリアの演出の流れ
		 * @details ・ゴールに触れたら Start、毎フレーム Update で時間を進める
		 *          ・どの扉をどれだけ開くか（GetDoorOpenRate）と、丸がどれだけ閉じたか（GetIrisCloseRate）を答える。
		 *            扉を描くのは GoalGateRenderer、丸を描くのは core::IrisWipe（このクラスは時間を数えるだけ）
		 *          ・IsFinished が true になったら、シーンが次のステージへ移る（DebugScene::RequestScene）
		 *
		 *          時間の流れ（ClearTiming）
		 *            0 ─ 扉が開く ─ doorOpenTime ─ 待つ ─ 丸が閉じる ─ 真っ暗で待つ ─ 終わり
		 */
		class ClearSequence
		{
		public:
			ClearSequence() = default;
			~ClearSequence() = default;

			/**
			 * @brief 初期化
			 * @param timing 時間（ResetTiming で戻る値にもなる）
			 */
			void Init(const ClearTiming& timing = ClearTiming());
			/**
			 * @brief 演出を始める（触れたゴールの扉を開く）
			 */
			void Start(const stage::Goal& goal);
			/**
			 * @brief 扉を開かずに、丸を閉じるところから始める（デバッグの Clear This Stage 用）
			 */
			void Start();
			/** 演出をやめて、最初の状態に戻す（リスタート・ステージの読み直し） */
			void Reset();
			/** 時間を進める（始まっていなければ何もしない） */
			void Update(float deltaTime);

			/** 演出が始まっているか */
			bool IsStarted() const { return m_isStarted; }
			/** 演出が全部終わったか（次のステージへ移ってよいか） */
			bool IsFinished() const;
			/**
			 * @brief そのゴールの扉の開き具合（0 = 閉じている 〜 1 = 開ききった。触れたゴール以外は 0）
			 */
			float GetDoorOpenRate(const stage::Goal& goal) const;
			/** 丸の閉じ具合（0 = 開いている 〜 1 = 閉じきって真っ暗） */
			float GetIrisCloseRate() const;

			// ---- 調整値 ----
			const ClearTiming& GetTiming() const { return m_timing; }
			ClearTiming& GetTimingForEdit() { return m_timing; }
			/** 時間を Init で渡された値に戻す */
			void ResetTiming() { m_timing = m_initialTiming; }

		private:
			/** 時間 */
			ClearTiming m_timing;
			/** Init で渡された時間 */
			ClearTiming m_initialTiming;
			/** 触れたゴール（扉を開くゴールの印。中身は読まない。扉を開かないときは nullptr） */
			const stage::Goal* m_goal = nullptr;
			/** 始まってからの時間 */
			float m_elapsed = 0.0f;
			/** 始まっているか */
			bool m_isStarted = false;
		};
	}
}
