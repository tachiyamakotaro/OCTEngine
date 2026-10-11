/**
 * @file CharacterStateMachine.cpp
 * @brief キャラクターのステートマシン基底クラス
 */
#include "stdafx.h"
#include "CharacterStateMachine.h"
#include "CharacterBase.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace actor
	{
		const char* CharacterStateMachine::GetCurrentStateName() const
		{
			return m_currentState != nullptr ? m_currentState->GetName() : "None";
		}


		void CharacterStateMachine::UpdateState(const float deltaTime)
		{
			PreUpdate(deltaTime);

			if (m_currentState == nullptr)
			{
				return;
			}
			m_stateTime += deltaTime;
			m_currentState->Update(deltaTime);
		}


		void CharacterStateMachine::UpdateTransition()
		{
			if (m_currentState == nullptr)
			{
				return;
			}

			// 死亡などの共通遷移を優先する
			int nextStateID = CheckGlobalTransition();
			if (nextStateID == STATE_ID_NONE)
			{
				nextStateID = m_currentState->CheckTransition();
			}

			if (nextStateID != STATE_ID_NONE && nextStateID != m_currentStateID)
			{
				ChangeState(nextStateID);
			}
		}


		void CharacterStateMachine::ChangeState(const int stateID)
		{
			auto it = m_states.find(stateID);
			if (it == m_states.end())
			{
				// 登録されていないステートに遷移しようとした
				return;
			}

			if (m_currentState != nullptr)
			{
				m_currentState->Exit();
			}
			m_previousStateID = m_currentStateID;
			m_currentStateID = stateID;
			m_currentState = it->second.get();
			m_stateTime = 0.0f;
			m_currentState->Enter();
		}


		void CharacterStateMachine::ApplyHorizontalMove(
			const float moveX, const float maxSpeed,
			const float acceleration, const float deceleration, const float deltaTime)
		{
			CharacterController& controller = m_owner->GetController();
			math::Vec2 velocity = controller.GetVelocity();

			const float targetSpeed = moveX * maxSpeed;
			const bool hasInput = math::Abs(moveX) > MOVE_INPUT_DEADZONE;
			// 入力と同じ向きに進んでいる時だけ加速度を使う（止まる・切り返す時は減速度）
			const bool isSameDirection = velocity.x * targetSpeed >= 0.0f;
			const float rate = (hasInput && isSameDirection) ? acceleration : deceleration;

			velocity.x = math::MoveTowards(velocity.x, targetSpeed, rate * deltaTime);
			controller.SetVelocity(velocity);
		}


		void CharacterStateMachine::RegisterState(const int stateID, std::unique_ptr<ICharacterState> state)
		{
			m_states[stateID] = std::move(state);
		}


		CharacterStateMachine::CharacterStateMachine(CharacterBase* owner)
			: m_owner(owner)
			, m_currentState(nullptr)
			, m_currentStateID(STATE_ID_NONE)
			, m_previousStateID(STATE_ID_NONE)
			, m_stateTime(0.0f)
		{}
	}
}
