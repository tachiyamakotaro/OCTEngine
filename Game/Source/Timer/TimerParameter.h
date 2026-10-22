/**
 * @file TimerParameter.h
 * @brief 時間制限の調整値（制限時間の初期値・警告を出す残り時間・時間切れの演出の長さ）
 * @details ステージごとの制限時間は StageCatalog.h の STAGE_INFO_TABLE の4つ目の値で決める。
 *          書いていない（0 の）ステージは DEFAULT_TIME_LIMIT を使う。
 */
#pragma once


namespace app
{
	namespace timer
	{
		/** 制限時間の初期値（秒。STAGE_INFO_TABLE に書いていないステージで使う） */
		constexpr float DEFAULT_TIME_LIMIT = 180.0f;

		/** 残りがこの秒数以下になったら「注意」（数字がオレンジになる） */
		constexpr float CAUTION_TIME = 30.0f;
		/** 残りがこの秒数以下になったら「危険」（数字が赤くなり、1秒ごとに跳ねて音が鳴る） */
		constexpr float DANGER_TIME = 10.0f;

		/** 時間切れになってから、ステージを最初からやり直すまでの時間（秒。TIME UP を見せる時間） */
		constexpr float TIME_UP_WAIT = 2.5f;
	}
}
