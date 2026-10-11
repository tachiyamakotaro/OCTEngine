/**
 * @file ICharacterState.h
 * @brief キャラクターのステートのインターフェース
 */
#pragma once


namespace app
{
	namespace actor
	{
		/** 「遷移しない」を表すステートID */
		constexpr int STATE_ID_NONE = -1;


		/**
		 * @brief キャラクターのステート
		 * @details 1フレームの流れ（CharacterBase::UpdateLogic）
		 *          1. Update()          … 速度を決める（まだ動かさない）
		 *          2. CharacterController::Move() … 実際に動かして当たり判定を解決
		 *          3. CheckTransition() … 動いた結果（着地した・崖が見つかった等）を見て次のステートを決める
		 */
		class ICharacterState
		{
		public:
			virtual ~ICharacterState() = default;


		public:
			/** ステートに入ったとき */
			virtual void Enter() {}
			/** 毎フレームの処理（移動前） */
			virtual void Update(float /*deltaTime*/) {}
			/** ステートから出るとき */
			virtual void Exit() {}
			/**
			 * @brief 遷移先を決める（移動後に呼ばれる）
			 * @return 遷移先のステートID。遷移しないなら STATE_ID_NONE
			 */
			virtual int CheckTransition() { return STATE_ID_NONE; }
			/**
			 * @brief ステート名（デバッグ表示用）
			 */
			virtual const char* GetName() const = 0;
		};
	}
}
