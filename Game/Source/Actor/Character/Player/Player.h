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
			GrabIdle,	//!< 物をつかんで止まっている
			Push,		//!< 物を押す（鏡を回すときもこれ）
			Pull,		//!< 物を引く
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
			// ---- 物の操作（鏡・日よけ板）----
			/**
			 * @brief 物をつかんで引きずっているかを設定する
			 * @param isGrabbing つかんで引きずっているか
			 * @param objectOffsetX つかんだ物の中心 - プレイヤーの X（物が右にあれば正）
			 * @details シーンが毎フレーム、鏡・日よけ板の操作を更新したあとに呼ぶ。
			 *          プレイヤーのステートは次のフレームにこれを見て「つかむ」に切り替わる
			 */
			void SetGrabbing(bool isGrabbing, float objectOffsetX);
			/** @brief 物をつかんで引きずっているか */
			inline bool IsGrabbing() const { return m_isGrabbing; }
			/** @brief つかんでいる物がどちら側にあるか（右なら 1、左なら -1） */
			inline int GetGrabObjectSide() const { return m_grabObjectOffsetX >= 0.0f ? 1 : -1; }
			/**
			 * @brief 鏡を回したことを知らせる（しばらく「回す」アニメーションにする）
			 */
			void NotifyObjectRotated();
			/** @brief 鏡を回す動きの途中か */
			inline bool IsRotatingObject() const { return m_rotateActionTimer > 0.0f; }


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
			/** 物をつかんで引きずっているか（シーンから設定される） */
			bool m_isGrabbing;
			/** つかんでいる物の中心 - プレイヤーの X */
			float m_grabObjectOffsetX;
			/** 鏡を回す動きの残り時間 */
			float m_rotateActionTimer;
		};
	}
}
