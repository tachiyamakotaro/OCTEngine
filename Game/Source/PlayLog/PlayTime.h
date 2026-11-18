/**
 * @file PlayTime.h
 * @brief プレイ時間（秒）を表す値
 * @details float の秒数をそのまま渡し回すと「残り時間なのか経過時間なのか」「何の単位か」が分からなくなるので、
 *          プレイログで使う時間はこの型で包む（プリミティブ型執着を避ける）。
 *          足し算と、ログに書くための文字列（例：1:23.45）への変換と、続きのために保存する秒数だけを持つ。
 */
#pragma once
#include <string>


namespace app
{
	namespace playlog
	{
		/**
		 * @brief プレイ時間（経過時間。マイナスにはならない）
		 */
		class PlayTime
		{
		public:
			PlayTime() = default;
			/** @param seconds 秒（マイナスなら 0 にする） */
			explicit PlayTime(float seconds);

			PlayTime operator+(const PlayTime& other) const;
			PlayTime& operator+=(const PlayTime& other);

			/** ログに書く文字列（分:秒.小数2桁。例：83.45 秒 → "1:23.45"） */
			std::string ToText() const;
			/** 秒（続きから遊ぶための記録に書くときだけ使う） */
			float ToSeconds() const { return m_seconds; }


		private:
			/** 秒 */
			float m_seconds = 0.0f;
		};
	}
}
