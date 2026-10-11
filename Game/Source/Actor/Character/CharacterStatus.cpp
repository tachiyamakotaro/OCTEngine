/**
 * @file CharacterStatus.cpp
 * @brief キャラクターのステータス基底クラス
 */
#include "stdafx.h"
#include "CharacterStatus.h"
#include "CharacterParameter.h"


namespace app
{
	namespace actor
	{
		CharacterStatus::CharacterStatus()
			: m_hp(1)
			, m_maxHp(1)
		{}


		void CharacterStatus::Setup(const CharacterParameter& param)
		{
			m_maxHp = param.maxHp;
			m_hp = m_maxHp;
		}


		void CharacterStatus::Update(const float /*deltaTime*/)
		{}


		bool CharacterStatus::ApplyDamage(const int damage)
		{
			if (IsDead() || damage <= 0)
			{
				return false;
			}
			m_hp -= damage;
			if (m_hp < 0)
			{
				m_hp = 0;
			}
			return true;
		}


		void CharacterStatus::Heal(const int amount)
		{
			if (IsDead() || amount <= 0)
			{
				return;
			}
			m_hp += amount;
			if (m_hp > m_maxHp)
			{
				m_hp = m_maxHp;
			}
		}


		void CharacterStatus::Kill()
		{
			m_hp = 0;
		}
	}
}
