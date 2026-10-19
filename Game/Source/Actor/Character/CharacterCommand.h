/**
 * @file CharacterCommand.h
 * @brief キャラクターへの「こう動きたい」という命令
 * @details ステートはボタンではなくこのコマンドを見て動く。
 *          ・プレイヤー  … PlayerController が入力から作る
 *          ・鏡の中の自分 … PlayerController が左右反転して作る
 *          ・エネミー    … AI（今後作る EnemyController など）が作る
 *          こうしておくと、同じステートマシンを入力元に関係なく使い回せる。
 */
#pragma once


namespace app
{
	namespace actor
	{
		/** 左右入力を「入力あり」とみなすしきい値 */
		constexpr float MOVE_INPUT_DEADZONE = 0.2f;


		/**
		 * @brief キャラクターコマンド
		 */
		struct CharacterCommand
		{
			/** 左右の移動入力（-1.0〜1.0） */
			float moveX = 0.0f;
			/** 上を押している */
			bool up = false;
			/** 下を押している */
			bool down = false;
			/** ジャンプを押した瞬間 */
			bool jumpTrigger = false;
			/** ジャンプを押し続けている */
			bool jumpHold = false;
			/** つかむを押している */
			bool grabHold = false;
			/** アクションを押した瞬間 */
			bool actionTrigger = false;


			/**
			 * @brief 左右入力があるか
			 */
			bool HasMoveInput() const
			{
				return moveX > MOVE_INPUT_DEADZONE || moveX < -MOVE_INPUT_DEADZONE;
			}
			/**
			 * @brief 全部クリア
			 */
			void Clear()
			{
				*this = CharacterCommand();
			}
		};
	}
}
