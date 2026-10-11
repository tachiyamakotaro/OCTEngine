/**
 * @file Player.h
 * @brief プレイヤー（探検家）
 * @details CharacterBase を継承し、プレイヤー固有のものをそろえる。
 *          ・PlayerParameter     … 調整値
 *          ・PlayerStatus        … HP + 無敵時間
 *          ・PlayerStateMachine  … 立ち/走り/ジャンプ/崖つかまり/崖登り…
 *          ・PlayerController    … 入力 → コマンド
 *
 *          鏡の中のもう一人の自分も、このクラスを SetMirrored(true) で作れば
 *          同じ入力で左右反転して動く（ミラーパラレルの同時操作）。
 */
#pragma once
#include "Source/Actor/Character/CharacterBase.h"
#include "PlayerController.h"
#include "PlayerParameter.h"


namespace app
{
	namespace actor
	{
		/**
		 * @brief プレイヤーのアニメーション番号
		 * @note PLAYER_ANIMATION_DATA（Player.cpp）と並びを合わせること
		 */
		enum class PlayerAnimation : int
		{
			Idle,
			Run,
			Jump,
			Fall,
			Crouch,
			LedgeHang,
			ClimbUp,
			HardLand,
			Dead,
			Num
		};


		/**
		 * @brief プレイヤー
		 */
		class Player : public CharacterBase
		{
		public:
			/**
			 * @brief 入力デバイスを設定（Start より前に呼ぶ）
			 */
			void SetInputDevice(const input::IInputDevice* device);
			/**
			 * @brief 鏡の中のプレイヤーにする（左右の入力が反転し、最初は左を向く）
			 */
			void SetMirrored(bool isMirrored);
			inline bool IsMirrored() const { return m_playerController.IsMirrorX(); }
			/**
			 * @brief パラメーターを差し替える（Start より前に呼ぶ）
			 */
			inline void SetPlayerParameter(const PlayerParameter& param) { m_parameter = param; }
			inline const PlayerParameter& GetPlayerParameter() const { return m_parameter; }
			/**
			 * @brief パラメーターを直接編集する（ImGui での調整用）
			 * @note 体の大きさ・重力はコントローラーに反映し直す必要がある（Game::ApplyTuning 参照）
			 */
			inline PlayerParameter& GetPlayerParameterForEdit() { return m_parameter; }
			/**
			 * @brief プレイヤーコントローラーを取得（演出中に入力を止めるなど）
			 */
			inline PlayerController& GetPlayerController() { return m_playerController; }


		public:
			bool Start() override;


		public:
			Player();
			~Player() override = default;


		protected:
			const CharacterParameter& GetParameter() const override { return m_parameter; }
			std::unique_ptr<CharacterStatus> CreateStatus() override;
			std::unique_ptr<CharacterStateMachine> CreateStateMachine() override;
			void UpdateCommand(float deltaTime) override;


		private:
			/** パラメーター */
			PlayerParameter m_parameter;
			/** 入力 → コマンド */
			PlayerController m_playerController;
		};
	}
}
