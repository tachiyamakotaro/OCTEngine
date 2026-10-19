/**
 * @file PlayerController.h
 * @brief プレイヤーの入力をキャラクターコマンドに変換するクラス
 * @details 入力デバイス → CharacterCommand の変換だけを担当する（体を動かすのは CharacterController）。
 *          ・「押した瞬間」の判定を前フレームとの比較で作る
 *          ・左右反転モード：鏡の中の自分は左右の入力が逆になる（ミラーパラレルの同時操作）
 *          ・入力の受付を止められる（演出中・ポーズ中など）
 */
#pragma once
#include "Source/Actor/Character/CharacterCommand.h"


namespace app
{
	namespace input
	{
		class IInputDevice;
	}


	namespace actor
	{
		/**
		 * @brief プレイヤーコントローラー
		 */
		class PlayerController
		{
		public:
			/**
			 * @brief 入力デバイスを設定（複数のプレイヤーで同じデバイスを共有してよい）
			 */
			inline void SetInputDevice(const input::IInputDevice* device) { m_device = device; }
			/**
			 * @brief 左右の入力を反転するか（鏡の中の自分用）
			 */
			inline void SetMirrorX(const bool isMirror) { m_isMirrorX = isMirror; }
			inline bool IsMirrorX() const { return m_isMirrorX; }
			/**
			 * @brief 入力を受け付けるか
			 */
			inline void SetEnabled(const bool isEnabled) { m_isEnabled = isEnabled; }
			inline bool IsEnabled() const { return m_isEnabled; }


		public:
			/**
			 * @brief 入力からコマンドを作る（毎フレーム1回）
			 * @param outCommand 書き込み先
			 */
			void Update(CharacterCommand& outCommand);


		public:
			explicit PlayerController(const input::IInputDevice* device = nullptr);


		private:
			/** 入力デバイス */
			const input::IInputDevice* m_device;
			/** 左右反転 */
			bool m_isMirrorX;
			/** 入力を受け付けるか */
			bool m_isEnabled;
			/** 前フレームにジャンプを押していたか */
			bool m_wasJumpPressed;
			/** 前フレームにアクションを押していたか */
			bool m_wasActionPressed;
		};
	}
}
