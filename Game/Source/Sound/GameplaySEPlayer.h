/**
 * @file GameplaySEPlayer.h
 * @brief インゲームの SE（プレイヤーの動き・ダメージ・ギミック）を鳴らす
 * @details プレイヤーやステージのクラスには音のことを書かず、毎フレーム「前のフレームから何が変わったか」を見て鳴らす。
 *          （Player / Stage は SoundManager を知らないままなので、テストやエディタからもそのまま使える）
 *
 *          鳴らすもの
 *          ・プレイヤー：足音（走っている間、一定の間隔）・ジャンプ・着地・よろけ着地・崖つかまり・崖登り・ダメージ・死亡
 *          ・反射鏡    ：回した・つかんだ・引きずっている間（ループ）
 *          ・光        ：受光器の点灯/消灯・光の足場が出た・合図のランプが光った
 *          ・扉など    ：扉が開き始めた/閉まり始めた・スイッチを踏んだ・霧が吹き出した・鏡の足場が見えた
 *          ワープ・チェックポイント・ゴール・リスタートは、起きた場所（DebugScene）から Play を呼ぶ。
 *
 *          使い方（DebugScene）
 *          ・Start とリスタートのあとに Reset（今の状態を覚えるだけで、音は鳴らさない）
 *          ・毎フレーム、ステージ → プレイヤー → 鏡の操作 の更新が終わったあとに Update
 *          鳴らす音の種類とファイルは Source/Sound/Types.h。
 */
#pragma once
#include <vector>
#include "Source/Sound/SoundHandle.h"


namespace app
{
	namespace actor
	{
		class Player;
	}
	namespace stage
	{
		class Stage;
		class MirrorInteractor;
	}


	/**
	 * @brief インゲームの SE を鳴らす
	 */
	class GameplaySEPlayer
	{
	public:
		// ---- 調整値（デバッグウィンドウの Sound で変えられる）----
		/** SE を鳴らすか */
		bool isEnabled = true;
		/** 足音の間隔（秒） */
		float footstepInterval = 0.3f;
		/** 走り始めてから最初の足音までの時間（秒） */
		float firstFootstepDelay = 0.05f;
		/** これより遅いときは足音を鳴らさない（壁に押し付けているときなど） */
		float minFootstepSpeed = 50.0f;
		/** これより短い時間しか空中にいなかったら着地の音を鳴らさない（段差を降りただけのときなど） */
		float minAirTimeForLand = 0.12f;
		/** 鏡が止まってから引きずる音を止めるまでの時間（秒。少し待たないと音がぶつぶつ切れる） */
		float slideStopDelay = 0.1f;


	public:
		/**
		 * @brief 今の状態を覚え直す（音は鳴らさない）
		 * @details Start のあと・リスタートのあと・ステージを作り直したあとに呼ぶ。
		 *          呼ばないと、扉が閉じた・受光器が消えたなどの音がまとめて鳴ってしまう。
		 */
		void Reset(const stage::Stage& stage, const actor::Player& player);
		/**
		 * @brief 前のフレームからの変化を見て SE を鳴らす（毎フレーム）
		 * @param deltaTime 経過時間
		 * @param stage ステージ（Update が終わったもの）
		 * @param player プレイヤー（Update が終わったもの）
		 * @param mirrorInteractor 反射鏡の操作（Update が終わったもの）
		 */
		void Update(float deltaTime, const stage::Stage& stage, const actor::Player& player, const stage::MirrorInteractor& mirrorInteractor);
		/**
		 * @brief SE を鳴らす（同じフレームに同じ音は1回だけ）
		 * @param kind enSoundKind
		 */
		void Play(int kind);
		/** @brief 鳴り続けている SE（鏡を引きずる音）を止める */
		void StopLoops();


	public:
		// ---- 確認用 ----
		/** @brief 最後に鳴らした SE（なければ enSoundKind_None） */
		inline int GetLastPlayedKind() const { return m_lastPlayedKind; }
		/** @brief 鳴らした回数 */
		inline int GetPlayCount() const { return m_playCount; }
		/** @brief 鏡を引きずる音が鳴っているか */
		bool IsSlidePlaying() const;


	public:
		GameplaySEPlayer();
		~GameplaySEPlayer();
		GameplaySEPlayer(const GameplaySEPlayer&) = delete;
		GameplaySEPlayer& operator=(const GameplaySEPlayer&) = delete;


	private:
		/** プレイヤーの動き・ダメージ */
		void UpdatePlayer(float deltaTime, const actor::Player& player);
		/** ステージのギミック */
		void UpdateStage(const stage::Stage& stage);
		/** 反射鏡の操作 */
		void UpdateMirror(float deltaTime, const stage::MirrorInteractor& mirrorInteractor);
		/** プレイヤーの今の状態を覚える */
		void TakePlayerSnapshot(const actor::Player& player);
		/** ステージの今の状態を覚える */
		void TakeStageSnapshot(const stage::Stage& stage);
		/** 覚えているギミックの数が今のステージと同じか（エディタで足したり消したりしたら違う） */
		bool IsSameLayout(const stage::Stage& stage) const;


	private:
		// ---- プレイヤー ----
		/** 前のフレームのステート（PlayerStateID。まだ覚えていなければ -1） */
		int m_prevStateID;
		/** 前のフレームのHP */
		int m_prevHp;
		/** 空中にいる時間 */
		float m_airTime;
		/** 次の足音までの時間 */
		float m_footstepTimer;
		/** 前のフレームに走っていたか（足音用） */
		bool m_wasRunning;

		// ---- ステージ（前のフレームの状態。並びはステージの配列と同じ）----
		std::vector<bool> m_receiverLit;
		std::vector<bool> m_switchPressed;
		std::vector<bool> m_lightPlatformSolid;
		std::vector<bool> m_indicatorOn;
		std::vector<float> m_doorOpenAmount;
		/** 扉が最後に動いた向き（1=開く、-1=閉まる、0=まだ動いていない） */
		std::vector<int> m_doorDirection;
		/** 霧の装置の数 */
		size_t m_mistCount;
		/** 鏡の足場の数 */
		size_t m_mirrorPlatformCount;
		/** 次の Update でステージの状態を覚え直すだけにするか（リセットの直後） */
		bool m_isStageSnapshotPending;

		// ---- 反射鏡 ----
		/** 引きずる音 */
		SEHandle m_slideHandle;
		/** 鏡が止まってからの時間 */
		float m_slideIdleTimer;

		// ---- 鳴らした音 ----
		/** このフレームに鳴らした音（同じ音を重ねないため） */
		std::vector<int> m_playedThisFrame;
		/** 最後に鳴らした音 */
		int m_lastPlayedKind;
		/** 鳴らした回数 */
		int m_playCount;
	};
}
