/**
 * @file CharacterStateMachine.h
 * @brief キャラクターのステートマシン基底クラス
 * @details ステートの登録・切り替え・共通の移動計算を受け持つ。
 *          プレイヤー/エネミーごとに派生クラスを作り、Setup() でステートを登録する。
 */
#pragma once
#include <memory>
#include <unordered_map>
#include "ICharacterState.h"


namespace app
{
	namespace actor
	{
		class CharacterBase;


		/**
		 * @brief キャラクターのステートマシン基底クラス
		 */
		class CharacterStateMachine
		{
		public:
			/**
			 * @brief 現在のステートIDを取得
			 */
			inline int GetCurrentStateID() const { return m_currentStateID; }
			/**
			 * @brief 1つ前のステートIDを取得
			 */
			inline int GetPreviousStateID() const { return m_previousStateID; }
			/**
			 * @brief 現在のステートに入ってからの経過時間（秒）
			 */
			inline float GetStateTime() const { return m_stateTime; }
			/**
			 * @brief 現在のステート名（デバッグ表示用）
			 */
			const char* GetCurrentStateName() const;
			/**
			 * @brief 持ち主のキャラクターを取得
			 */
			inline CharacterBase* GetOwner() const { return m_owner; }


		public:
			/**
			 * @brief ステートの登録と、最初のステートへの遷移を行う
			 * @note 派生クラスで実装する
			 */
			virtual void Setup() = 0;
			/**
			 * @brief ステートの更新（移動前）
			 */
			void UpdateState(float deltaTime);
			/**
			 * @brief ステートの遷移判定（移動後）
			 */
			void UpdateTransition();
			/**
			 * @brief ステートを切り替える
			 * @param stateID 切り替え先
			 */
			void ChangeState(int stateID);


		public:
			/**
			 * @brief 左右移動の速度を計算してキャラクターコントローラーに設定する
			 * @details 目標速度 = moveX * maxSpeed に向かって、加速度/減速度で近づける。
			 *          入力と逆向きに動いているとき（切り返し）は減速度を使うので素早く止まる。
			 * @param moveX 左右入力（-1〜1）
			 * @param maxSpeed 最高速度
			 * @param acceleration 加速度
			 * @param deceleration 減速度
			 * @param deltaTime 経過時間
			 */
			void ApplyHorizontalMove(float moveX, float maxSpeed, float acceleration, float deceleration, float deltaTime);


		protected:
			/**
			 * @brief ステートを登録する
			 */
			void RegisterState(int stateID, std::unique_ptr<ICharacterState> state);
			/**
			 * @brief どのステートからでも優先される遷移（死亡など）
			 * @return 遷移先。なければ STATE_ID_NONE
			 */
			virtual int CheckGlobalTransition() { return STATE_ID_NONE; }
			/**
			 * @brief ステート更新の前に毎フレーム呼ばれる共通処理（タイマー更新など）
			 */
			virtual void PreUpdate(float /*deltaTime*/) {}


		public:
			explicit CharacterStateMachine(CharacterBase* owner);
			virtual ~CharacterStateMachine() = default;


		protected:
			/** 持ち主 */
			CharacterBase* m_owner;


		private:
			/** 登録されたステート */
			std::unordered_map<int, std::unique_ptr<ICharacterState>> m_states;
			/** 現在のステート */
			ICharacterState* m_currentState;
			/** 現在のステートID */
			int m_currentStateID;
			/** 1つ前のステートID */
			int m_previousStateID;
			/** 現在のステートの経過時間 */
			float m_stateTime;
		};
	}
}
