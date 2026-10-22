/**
 * @file Types.h
 * @brief サウンドの種類とファイルパス・音量
 * @details 種類を足すときは enSoundKind と GetSoundAssetPath の両方に足す（音量を変えたいなら GetSoundDefaultVolume にも）。
 *          ファイルがないサウンドは鳴らないだけ（ゲームは止まらない）。
 *          UI と BGM のパスは ProjectBeast と同じにしてあるので、あちらの Assets/sound をコピーすれば鳴る。
 *          プレイヤーとギミックの SE は Assets/sound/SE/player/ と Assets/sound/SE/gimmick/ に置く。
 */
#pragma once


namespace app
{
	enum enSoundKind
	{
		// =========================================================================
		// SE (効果音)
		// =========================================================================
		enSoundKind_SE = 0,
		// --- UI: 汎用操作 ---
		enSoundKind_ButtonBack = enSoundKind_SE,
		enSoundKind_ButtonEnter,
		enSoundKind_CursorMove,

		// --- プレイヤー ---
		enSoundKind_PlayerFootstep,		// 走っている間の足音（一定の間隔で鳴らす）
		enSoundKind_PlayerJump,			// ジャンプ
		enSoundKind_PlayerLand,			// 着地
		enSoundKind_PlayerHardLand,		// 高いところから着地してよろける
		enSoundKind_PlayerLedgeGrab,	// 崖につかまる
		enSoundKind_PlayerClimbUp,		// 崖を登る
		enSoundKind_PlayerDamage,		// ダメージを受けた
		enSoundKind_PlayerDeath,		// 死亡（HP0・奈落）
		enSoundKind_PlayerRespawn,		// リスタートして再開した

		// --- ギミック：反射鏡 ---
		enSoundKind_MirrorRotate,		// 反射鏡を回した
		enSoundKind_MirrorGrab,			// 反射鏡をつかんだ
		enSoundKind_MirrorSlide,		// 反射鏡を引きずっている間（ループ）

		// --- ギミック：光 ---
		enSoundKind_ReceiverOn,			// 受光器に光が当たった
		enSoundKind_ReceiverOff,		// 受光器から光が外れた
		enSoundKind_LightPlatformOn,	// 光の足場が出た
		enSoundKind_IndicatorOn,		// 合図のランプが光った

		// --- ギミック：扉・スイッチ・霧 ---
		enSoundKind_DoorOpen,			// 扉が開き始めた
		enSoundKind_DoorClose,			// 扉が閉まり始めた
		enSoundKind_SwitchOn,			// 床スイッチを踏んだ
		enSoundKind_MistBurst,			// 霧が吹き出した
		enSoundKind_MirrorPlatformReveal,	// 鏡の足場が見えた

		// --- 進行 ---
		enSoundKind_Warp,				// ワープした
		enSoundKind_Checkpoint,			// 新しいチェックポイントに触れた
		enSoundKind_Goal,				// ゴールした
		enSoundKind_TimerTick,			// 時間制限：残りわずかのとき1秒ごとに鳴る「ピッ」
		enSoundKind_TimeUp,				// 時間制限：時間切れ

		enSoundKind_SE_Max,

		// =========================================================================
		// BGM
		// =========================================================================
		enSoundKind_BGM = enSoundKind_SE_Max,
		enSoundKind_InGame = enSoundKind_BGM,
		enSoundKind_Title,
		enSoundKind_Result,

		// =========================================================================
		// 全体の最大数定義
		// =========================================================================
		enSoundKind_Max,
		enSoundKind_None = enSoundKind_Max,
	};


	/** サウンドのファイルパス（Game フォルダから） */
	inline const char* GetSoundAssetPath(const int kind)
	{
		switch (kind)
		{
		// --- UI: 汎用操作 ---
		case enSoundKind_ButtonBack:			return "Assets/sound/SE/controller/buttonBack.wav";
		case enSoundKind_ButtonEnter:			return "Assets/sound/SE/controller/buttonEnter.wav";
		case enSoundKind_CursorMove:			return "Assets/sound/SE/controller/cursorMove.wav";
		// --- プレイヤー ---
		case enSoundKind_PlayerFootstep:		return "Assets/sound/SE/player/footstep.wav";
		case enSoundKind_PlayerJump:			return "Assets/sound/SE/player/jump.wav";
		case enSoundKind_PlayerLand:			return "Assets/sound/SE/player/land.wav";
		case enSoundKind_PlayerHardLand:		return "Assets/sound/SE/player/hardLand.wav";
		case enSoundKind_PlayerLedgeGrab:		return "Assets/sound/SE/player/ledgeGrab.wav";
		case enSoundKind_PlayerClimbUp:			return "Assets/sound/SE/player/climbUp.wav";
		case enSoundKind_PlayerDamage:			return "Assets/sound/SE/player/damage.wav";
		case enSoundKind_PlayerDeath:			return "Assets/sound/SE/player/death.wav";
		case enSoundKind_PlayerRespawn:			return "Assets/sound/SE/player/respawn.wav";
		// --- ギミック：反射鏡 ---
		case enSoundKind_MirrorRotate:			return "Assets/sound/SE/gimmick/mirrorRotate.wav";
		case enSoundKind_MirrorGrab:			return "Assets/sound/SE/gimmick/mirrorGrab.wav";
		case enSoundKind_MirrorSlide:			return "Assets/sound/SE/gimmick/mirrorSlide.wav";
		// --- ギミック：光 ---
		case enSoundKind_ReceiverOn:			return "Assets/sound/SE/gimmick/receiverOn.wav";
		case enSoundKind_ReceiverOff:			return "Assets/sound/SE/gimmick/receiverOff.wav";
		case enSoundKind_LightPlatformOn:		return "Assets/sound/SE/gimmick/lightPlatformOn.wav";
		case enSoundKind_IndicatorOn:			return "Assets/sound/SE/gimmick/indicatorOn.wav";
		// --- ギミック：扉・スイッチ・霧 ---
		case enSoundKind_DoorOpen:				return "Assets/sound/SE/gimmick/doorOpen.wav";
		case enSoundKind_DoorClose:				return "Assets/sound/SE/gimmick/doorClose.wav";
		case enSoundKind_SwitchOn:				return "Assets/sound/SE/gimmick/switchOn.wav";
		case enSoundKind_MistBurst:				return "Assets/sound/SE/gimmick/mistBurst.wav";
		case enSoundKind_MirrorPlatformReveal:	return "Assets/sound/SE/gimmick/mirrorPlatformReveal.wav";
		// --- 進行 ---
		case enSoundKind_Warp:					return "Assets/sound/SE/progress/warp.wav";
		case enSoundKind_Checkpoint:			return "Assets/sound/SE/progress/checkpoint.wav";
		case enSoundKind_Goal:					return "Assets/sound/SE/progress/goal.wav";
		case enSoundKind_TimerTick:				return "Assets/sound/SE/progress/timerTick.wav";
		case enSoundKind_TimeUp:				return "Assets/sound/SE/progress/timeUp.wav";
		// --- BGM ---
		case enSoundKind_InGame:				return "Assets/sound/BGM/inGame.wav";
		case enSoundKind_Title:					return "Assets/sound/BGM/title.wav";
		case enSoundKind_Result:				return "Assets/sound/BGM/result.wav";
		default:								return nullptr;
		}
	}


	/**
	 * @brief サウンドごとの音量の倍率（PlaySE / PlayBGM で倍率を指定しなかったときに使う）
	 * @details 素材によって大きさがばらばらなので、ここでそろえる。何度も鳴る音（足音など）は小さめにする。
	 */
	inline float GetSoundDefaultVolume(const int kind)
	{
		switch (kind)
		{
		case enSoundKind_PlayerFootstep:		return 0.6f;
		case enSoundKind_MirrorSlide:			return 0.7f;
		case enSoundKind_ReceiverOff:			return 0.7f;
		default:								return 1.0f;
		}
	}
}
