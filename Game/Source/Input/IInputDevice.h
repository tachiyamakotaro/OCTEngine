/**
 * @file IInputDevice.h
 * @brief 入力デバイスのインターフェース
 * @details PlayerController は g_pad を直接触らず、このインターフェース越しに入力を読む。
 *          ・本番：GamePadInputDevice（g_pad を読む）
 *          ・テスト/リプレイ/デモ：決め打ちの入力を返す実装
 *          を差し替えられる。
 */
#pragma once
#include <cstdint>


namespace app
{
	namespace input
	{
		/**
		 * @brief ゲーム内のアクション（ボタン割り当てはデバイス側で決める）
		 */
		enum class InputAction : uint8_t
		{
			Left,		//!< 左移動
			Right,		//!< 右移動
			Up,			//!< 上（崖を登る）
			Down,		//!< 下（しゃがむ / 崖から手を離す）
			Jump,		//!< ジャンプ
			Grab,		//!< つかむ（自動つかまりOFFのとき使う）
			Action,		//!< 調べる・鏡を動かすなど
			Num
		};


		/**
		 * @brief 入力デバイス
		 */
		class IInputDevice
		{
		public:
			virtual ~IInputDevice() = default;


		public:
			/**
			 * @brief アクションが押されているか
			 * @note 「押した瞬間」の判定は PlayerController 側で前フレームと比較して作る
			 */
			virtual bool IsPress(InputAction action) const = 0;
			/**
			 * @brief 左右のアナログ入力（-1.0〜1.0）
			 * @details 既定はデジタル入力（Left/Right）から作る
			 */
			virtual float GetMoveX() const
			{
				return (IsPress(InputAction::Right) ? 1.0f : 0.0f) - (IsPress(InputAction::Left) ? 1.0f : 0.0f);
			}
		};
	}
}
