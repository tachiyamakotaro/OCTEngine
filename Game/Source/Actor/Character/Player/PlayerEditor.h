#pragma once


/**
 * @file PlayerEditor.h
 * @brief プレイヤーの状態の表示と、動きの調整値を変える ImGui（デバッグウィンドウの「Player」「Tuning」の中身）
 */
namespace app
{
	namespace actor
	{
		class Player;
		struct PlayerParameter;


		/**
		 * @brief プレイヤーの ImGui
		 * @details ・ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 *          ・状態は見るだけ（DrawStatus）、調整値は PlayerParameter だけを受け取って変える（DrawTuning）
		 */
		class PlayerEditor
		{
		public:
			PlayerEditor() = default;
			~PlayerEditor() = default;

			/**
			 * @brief 今の状態（ステート・HP・位置・速さ・接地）を表示する
			 */
			void DrawStatus(const Player& player);
			/**
			 * @brief 動きの調整値を変える
			 * @param param 変える調整値（Player::GetPlayerParameterForEdit）
			 */
			void DrawTuning(PlayerParameter& param);

		private:
			/** 走る・跳ぶ・落ちる・崖つかまり */
			void DrawMovement(PlayerParameter& param);
			/** モデルの向き・大きさと、体の大きさ */
			void DrawModelAndBody(PlayerParameter& param);
		};
	}
}
