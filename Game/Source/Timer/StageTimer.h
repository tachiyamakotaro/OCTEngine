/**
 * @file StageTimer.h
 * @brief ステージの時間制限（残り時間を数えるだけ。見た目は ui::TimerUI）
 * @details 使い方（DebugScene）
 *          ・Start(制限時間)        … ステージを始めたとき（残り時間を満タンにして動かす）
 *          ・Update(deltaTime)      … 毎フレーム（止まっている間・時間切れのあとは何もしない）
 *          ・SetRunning(false)      … ゴールしたとき・シーンの切り替え中など、数えたくない間（シーンが毎フレーム決める）
 *          ・SetPaused(true)        … 手で止める（デバッグの Stop Timer など）。SetPaused(false) するまで止まったまま
 *          ・IsTimeUpThisFrame()    … 時間切れになった瞬間だけ true（TIME UP の表示・音を出す合図）
 *
 *          ポーズ中は SceneManager がシーンの Update を呼ばないので、何もしなくても止まる。
 *          死んでチェックポイントから再開しても残り時間は戻さない（戻すと時間制限の意味がなくなるため）。
 *          残り時間を戻すのは、時間切れでステージを最初からやり直すときと、ステージを読み直したときだけ。
 *
 *          画面に出す秒数は「残り時間の切り上げ」（GetDisplaySeconds）。
 *          こうすると 3:00 から始まり、0:00 を表示するのは時間切れになった瞬間だけになる。
 */
#pragma once


namespace app
{
	namespace timer
	{
		/**
		 * @brief 残り時間の段階（数字の色・演出を変える）
		 */
		enum class TimeLevel
		{
			Normal,		//!< 余裕がある（白）
			Caution,	//!< 残りわずか（オレンジ）CAUTION_TIME 以下
			Danger,		//!< もうすぐ時間切れ（赤・1秒ごとに跳ねる・音）DANGER_TIME 以下
			TimeUp,		//!< 時間切れ
		};


		/**
		 * @brief ステージの時間制限
		 */
		class StageTimer
		{
		public:
			StageTimer();

			/**
			 * @brief 制限時間を決めて、最初から数え始める
			 * @param limitSeconds 制限時間（秒。0 以下なら時間制限なし＝ずっと減らない）
			 */
			void Start(float limitSeconds);
			/** 残り時間を満タンに戻して、また数え始める（制限時間はそのまま） */
			void Restart();
			/** 毎フレームの更新（止まっている間・時間切れのあとは何もしない） */
			void Update(float deltaTime);

			/** 数えるか（ゴールしたあと・フェード中などは false にして止める） */
			void SetRunning(bool isRunning) { m_isRunning = isRunning; }
			bool IsRunning() const { return m_isRunning; }

			/**
			 * @brief 手で止める・動かす（デバッグ用など）
			 * @details SetRunning とは別に覚えておくので、シーンが毎フレーム SetRunning(true) しても止まったまま。
			 *          Start / Restart でも元に戻さない（止めたまま時間を満タンにして確かめられるように）。
			 */
			void SetPaused(bool isPaused) { m_isPaused = isPaused; }
			bool IsPaused() const { return m_isPaused; }

			/** 残り時間を足す（マイナスで減らす。時間が増えるアイテム・デバッグ用。時間切れのあとは何もしない） */
			void AddTime(float seconds);
			/** 残り時間を直接決める（デバッグ用。0 にするとすぐ時間切れ。時間切れのあとは何もしない） */
			void SetRemaining(float seconds);

			/**
			 * @brief ImGui の調整項目（残り時間の表示・止める/動かす・時間を足す・時間切れにする）
			 * @details 見出し（CollapsingHeader）は呼ぶ側で作る。GameCamera::DrawSettingsUI と同じ使い方。
			 */
			void DrawSettingsUI();


		public:
			/** 時間制限があるか（制限時間が 0 以下なら false。UI も出さない） */
			bool IsEnabled() const { return m_limit > 0.0f; }
			/** 制限時間（秒） */
			float GetLimit() const { return m_limit; }
			/** 残り時間（秒。0 未満にはならない） */
			float GetRemaining() const { return m_remaining; }
			/** 経過時間（秒。クリアタイムとして使える） */
			float GetElapsed() const { return m_limit - m_remaining; }
			/** 残り時間の割合（満タンで 1、時間切れで 0。ゲージ用） */
			float GetRemainingRate() const;
			/** 画面に出す秒数（残り時間の切り上げ。例：残り 59.2 秒なら 60 → 1:00） */
			int GetDisplaySeconds() const;
			/** 今の段階（色・演出用） */
			TimeLevel GetLevel() const;

			/** 時間切れか */
			bool IsTimeUp() const { return m_isTimeUp; }
			/** このフレームで時間切れになったか */
			bool IsTimeUpThisFrame() const { return m_isTimeUpThisFrame; }
			/** このフレームで表示の秒数が変わったか（危険のときの「ピッ」の音・数字が跳ねる合図） */
			bool IsSecondChangedThisFrame() const { return m_isSecondChangedThisFrame; }


		private:
			/** 時間切れになったかを調べる（残り 0 になった瞬間に1回だけ立てる） */
			void CheckTimeUp();


		private:
			/** 制限時間（秒） */
			float m_limit;
			/** 残り時間（秒） */
			float m_remaining;
			/** 数えているか */
			bool m_isRunning;
			/** 手で止めているか */
			bool m_isPaused;
			/** 時間切れか */
			bool m_isTimeUp;
			/** このフレームで時間切れになったか */
			bool m_isTimeUpThisFrame;
			/** このフレームで表示の秒数が変わったか */
			bool m_isSecondChangedThisFrame;
		};
	}
}
