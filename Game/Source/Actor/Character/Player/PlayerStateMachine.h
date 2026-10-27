/**
 * @file PlayerStateMachine.h
 * @brief プレイヤーのステートマシン
 * @details ステートの登録と、複数のステートで使う共通処理を持つ。
 *          ・ジャンプの先行入力 / コヨーテタイム
 *          ・崖つかまりの判定と、つかんだ崖の情報
 *          ・落下距離（落下ダメージ用）
 *          ・地上/空中の左右移動
 */
#pragma once
#include "Source/Actor/Character/CharacterController.h"
#include "Source/Actor/Character/CharacterStateMachine.h"


namespace app
{
	namespace actor
	{
		class Player;
		struct PlayerParameter;
		struct CharacterCommand;


		/**
		 * @brief プレイヤーのステートID
		 */
		enum class PlayerStateID : int
		{
			Idle,		//!< 立ち
			Run,		//!< 走り
			Crouch,		//!< しゃがみ
			Jump,		//!< ジャンプ（上昇中）
			Fall,		//!< 落下中
			LedgeHang,	//!< 崖にぶら下がり
			ClimbUp,	//!< 崖を登る
			HardLand,	//!< 高いところから落ちてよろける
			Dead,		//!< 死亡
			Grab,		//!< 物（鏡・日よけ板）をつかんで動かす / 鏡を回す
			Num
		};


		/**
		 * @brief プレイヤーのステートマシン
		 */
		class PlayerStateMachine : public CharacterStateMachine
		{
		public:
			// ---- 便利な取得関数（ステートから使う）----
			Player& GetPlayer() const;
			const PlayerParameter& GetParam() const;
			const CharacterCommand& GetCommand() const;
			CharacterController& GetController() const;
			/**
			 * @brief PlayerStateID で切り替える
			 */
			void ChangeState(PlayerStateID stateID);
			using CharacterStateMachine::ChangeState;
			/**
			 * @brief 現在のステートIDを PlayerStateID で取得
			 */
			PlayerStateID GetCurrentPlayerStateID() const;


		public:
			// ---- ジャンプ ----
			/** @brief ジャンプの先行入力が残っているか */
			inline bool IsJumpBuffered() const { return m_jumpBufferTimer > 0.0f; }
			/** @brief 足場から落ちた直後のジャンプ猶予が残っているか */
			inline bool CanCoyoteJump() const { return m_coyoteTimer > 0.0f; }
			/** @brief ジャンプを使った（先行入力とコヨーテタイムを消費する） */
			void ConsumeJump();

			// ---- 崖つかまり ----
			/**
			 * @brief 今崖をつかもうとしているか（クールダウン・入力を見る）
			 */
			bool WantsToGrabLedge() const;
			/**
			 * @brief 向いている方向につかまれる崖があれば記録して true を返す
			 */
			bool TryFindLedge();
			/** @brief 最後に見つけた崖 */
			inline const LedgeInfo& GetLedge() const { return m_ledge; }
			/** @brief 崖から手を離した（しばらく再びつかまない） */
			void StartLedgeCooldown();

			// ---- 落下距離 ----
			/**
			 * @brief 最後に地面（または崖）にいた時の最高点から、今の位置までの落下距離
			 */
			float GetFallDistance() const;

			// ---- 移動 ----
			/** @brief 入力に合わせて向きを変える */
			void UpdateFacingFromInput();
			/** @brief 地上の左右移動 */
			void ApplyGroundMove(float deltaTime);
			/** @brief 空中の左右移動 */
			void ApplyAirMove(float deltaTime);
			/** @brief 左右の速度を止める方向に減速する */
			void ApplyStop(float deltaTime);
			/** @brief 物をつかんで引きずっているときの左右移動（ゆっくり） */
			void ApplyGrabMove(float deltaTime);


		public:
			void Setup() override;


		protected:
			int CheckGlobalTransition() override;
			void PreUpdate(float deltaTime) override;


		public:
			explicit PlayerStateMachine(Player* owner);
			~PlayerStateMachine() override = default;


		private:
			/** 持ち主のプレイヤー */
			Player* m_player;
			/** ジャンプ先行入力の残り時間 */
			float m_jumpBufferTimer;
			/** コヨーテタイムの残り時間 */
			float m_coyoteTimer;
			/** 崖を再びつかめるまでの残り時間 */
			float m_ledgeCooldownTimer;
			/** 最後に見つけた崖 */
			LedgeInfo m_ledge;
			/** 空中にいる間の最高点（落下距離の計算用） */
			float m_airPeakY;
		};
	}
}
