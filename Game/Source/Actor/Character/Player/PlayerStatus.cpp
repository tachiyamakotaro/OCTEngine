/**
 * @file PlayerStatus.cpp
 * @brief プレイヤーのステータス
 */
#include "stdafx.h"
#include "PlayerStatus.h"


namespace app
{
	namespace actor
	{
		void PlayerStatus::Update(const float deltaTime)
		{
			CharacterStatus::Update(deltaTime);

			if (m_invincibleTimer > 0.0f)
			{
				m_invincibleTimer -= deltaTime;
			}
		}


		bool PlayerStatus::ApplyDamage(const int damage)
		{
			if (IsInvincible())
			{
				return false;
			}
			if (!CharacterStatus::ApplyDamage(damage))
			{
				return false;
			}
			m_invincibleTimer = m_invincibleDuration;
			return true;
		}


		PlayerStatus::PlayerStatus(const float invincibleDuration)
			: m_invincibleDuration(invincibleDuration)
			, m_invincibleTimer(0.0f)
		{}
	}
}
