#pragma once
#include "Source/Stage/Start/StartParameter.h"


/**
 * @file StartSequence.h
 * @brief ステージ開始の演出の流れ（小さい丸から始まって、丸が外側へ開いていく）
 */
namespace app
{
	namespace stage
	{
		/**
		 * @brief ステージ開始の演出の流れ
		 * @details ・ClearSequence（ゴールの演出）の逆。丸が開いた黒い画像が外側へ広がって消えていく
		 *          ・このクラスは時間を数えて、丸の閉じ具合（GetIrisCloseRate）を答えるだけ
		 *            丸を描くのは core::IrisWipe、カメラを寄せる・引くのは camera::EventCamera（このクラスはどちらも知らない）
		 *          ・丸が開き始めた瞬間（IsOpenStartedThisFrame）に、シーンがカメラを引き始めさせて、操作できるようにする
		 *          ・画面の切り替え（SceneManager のフェード）の間は、シーンが deltaTime を 0 にして渡す
		 *            （真っ暗な間に時間が進んで、見る前に丸が開いてしまわないように）
		 *
		 *          時間の流れ（StartTiming）
		 *            Start ─ 小さい丸のまま待つ(holdTime) ─ 丸が開く(irisOpenTime) ─ 終わり
		 */
		class StartSequence
		{
		public:
			StartSequence() = default;
			~StartSequence() = default;

			/**
			 * @brief 初期化
			 * @param timing 時間（ResetTiming で戻る値にもなる）
			 */
			void Init(const StartTiming& timing = StartTiming());
			/** 演出を始める（小さい丸の状態から） */
			void Start();
			/** 演出をやめる（丸を消す。リスタート・ステージの読み直し） */
			void Reset();
			/**
			 * @brief 時間を進める（始まっていなければ何もしない）
			 * @param deltaTime 経過時間（秒。画面の切り替え中は 0 を渡す）
			 */
			void Update(float deltaTime);

			/** 演出中か（丸が開ききるまで） */
			bool IsPlaying() const { return m_isPlaying; }
			/** 丸が開き始める前か（この間は操作できない・時間制限を数えない） */
			bool IsHolding() const;
			/** このフレームで丸が開き始めたか（カメラを引き始め、操作できるようにする合図） */
			bool IsOpenStartedThisFrame() const { return m_isOpenStartedThisFrame; }
			/** 丸の閉じ具合（0 = 開いている 〜 1 = 真っ暗） */
			float GetIrisCloseRate() const;

			// ---- 調整値 ----
			const StartTiming& GetTiming() const { return m_timing; }
			StartTiming& GetTimingForEdit() { return m_timing; }
			/** 時間を Init で渡された値に戻す */
			void ResetTiming() { m_timing = m_initialTiming; }

		private:
			/** 時間 */
			StartTiming m_timing;
			/** Init で渡された時間 */
			StartTiming m_initialTiming;
			/** 始まってからの時間 */
			float m_elapsed = 0.0f;
			/** 演出中か */
			bool m_isPlaying = false;
			/** 丸が開き始めたか */
			bool m_hasOpenStarted = false;
			/** このフレームで丸が開き始めたか */
			bool m_isOpenStartedThisFrame = false;
		};
	}
}
