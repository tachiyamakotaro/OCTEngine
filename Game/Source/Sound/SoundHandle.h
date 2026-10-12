/**
 * @file SoundHandle.h
 * @brief サウンド再生ハンドルの型定義（軽量ヘッダ）
 */
#pragma once
#include <cstdint>


namespace app
{
	/** SE用のハンドル名 */
	using SEHandle = uint32_t;
	/** ハンドル無効値 */
	static constexpr SEHandle INVALID_SE_HANDLE = 0xffffffff;
}
