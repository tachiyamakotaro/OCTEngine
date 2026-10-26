/**
 * @file PlayerState.h
 * @brief プレイヤーの各ステート
 * @details 遷移図（主なもの）
 *
 *   Idle ⇄ Run          地上の移動
 *   Idle/Run → Crouch   下を押す（頭上が空けば立ち上がる）
 *   Idle/Run → Jump     ジャンプ（先行入力あり）
 *   Idle/Run → Fall     足場がなくなった（コヨーテタイム中はまだジャンプできる）
 *   Jump → Fall         上昇が終わった / 天井に頭をぶつけた
 *   Jump/Fall → LedgeHang  向いている方向に崖があった
 *   LedgeHang → ClimbUp    上 or ジャンプ
 *   LedgeHang → Fall       下 / つかむボタンを離した（自動つかまりOFF時）
 *   ClimbUp → Idle         登りきった
 *   Fall → Idle/Run        普通に着地
 *   Fall → HardLand        高いところから着地（ダメージ／死亡）
 *   どこからでも → Dead      HPが0（PlayerStateMachine::CheckGlobalTransition）
 */
#pragma once
#include "Source/Actor/Character/ICharacterState.h"
#include "PlayerStateMachine.h"


namespace app
{
	namespace actor
	{
		struct PlayerParameter;
		struct CharacterCommand;


		/**
		 * @brief プレイヤーのステートの基底クラス（よく使うものへの近道を用意する）
		 */
		class PlayerStateBase : public ICharacterState
		{
		public:
			explicit PlayerStateBase(PlayerStateMachine* machine) : m_machine(machine) {}
			~PlayerStateBase() override = default;


		protected:
			/** ステートIDを int にする */
			static int ToID(const PlayerStateID id) { return static_cast<int>(id); }

			Player& GetPlayer() const { return m_machine->GetPlayer(); }
			CharacterController& GetController() const { return m_machine->GetController(); }
			const CharacterCommand& GetCommand() const { return m_machine->GetCommand(); }
			const PlayerParameter& GetParam() const { return m_machine->GetParam(); }
			float GetStateTime() const { return m_machine->GetStateTime(); }
			/** アニメーション再生 */
			void PlayAnimation(int animation) const;
			/** 地上で、ジャンプ/しゃがみ/落下 のどれかに移るか（Idle と Run で共通） */
			int CheckGroundCommonTransition() const;


		protected:
			/** ステートマシン */
			PlayerStateMachine* m_machine;
		};




		/************************************/


		/** 立ち */
		class PlayerIdleState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			int CheckTransition() override;
			const char* GetName() const override { return "Idle"; }
		};


		/** 走り */
		class PlayerRunState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			int CheckTransition() override;
			const char* GetName() const override { return "Run"; }
		};


		/** しゃがみ */
		class PlayerCrouchState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			void Exit() override;
			int CheckTransition() override;
			const char* GetName() const override { return "Crouch"; }
		};


		/** ジャンプ（上昇中） */
		class PlayerJumpState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			int CheckTransition() override;
			const char* GetName() const override { return "Jump"; }

		private:
			/** 小ジャンプの減速を済ませたか */
			bool m_isJumpCut = false;
		};


		/** 落下中 */
		class PlayerFallState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			int CheckTransition() override;
			const char* GetName() const override { return "Fall"; }
		};


		/** 崖にぶら下がり */
		class PlayerLedgeHangState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			void Exit() override;
			int CheckTransition() override;
			const char* GetName() const override { return "LedgeHang"; }
		};


		/** 崖を登る */
		class PlayerClimbUpState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			void Exit() override;
			int CheckTransition() override;
			const char* GetName() const override { return "ClimbUp"; }

		private:
			/**
			 * @brief モデルを、ぶら下がった位置に置いたまま描くようにする（見た目だけ）
			 * @param carried 登り始めてから動く床に運ばれた分
			 */
			void UpdateClimbModel(const math::Vec2& carried) const;

		private:
			/** 登り始めの位置 */
			math::Vec2 m_startPosition;
			/** 登りきった位置 */
			math::Vec2 m_endPosition;
		};


		/** 高いところから落ちてよろける */
		class PlayerHardLandState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			int CheckTransition() override;
			const char* GetName() const override { return "HardLand"; }
		};


		/** 死亡 */
		class PlayerDeadState : public PlayerStateBase
		{
		public:
			using PlayerStateBase::PlayerStateBase;
			void Enter() override;
			void Update(float deltaTime) override;
			const char* GetName() const override { return "Dead"; }
		};
	}
}
