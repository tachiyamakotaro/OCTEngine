/**
 * @file PlayerStatus.h
 * @brief プレイヤーのステータス
 * @details キャラクター共通のHPに加えて、被ダメージ後の無敵時間を持つ。
 */
#pragma once
#include "Source/Actor/Character/CharacterStatus.h"


namespace app
{
	namespace actor
	{
		/**
		 * @brief プレイヤーのステータス
		 */
		class PlayerStatus : public CharacterStatus
		{
		public:
			/**
			 * @brief 無敵中か
			 */
			inline bool IsInvincible() const { return m_invincibleTimer > 0.0f; }
			/**
			 * @brief 無敵の残り時間（秒）
			 */
			inline float GetInvincibleTimer() const { return m_invincibleTimer; }


		public:
			void Update(float deltaTime) override;
			/**
			 * @brief ダメージを受ける（無敵中は受けない。受けたら無敵時間が始まる）
			 */
			bool ApplyDamage(int damage) override;


		public:
			/**
			 * @param invincibleDuration ダメージ後の無敵時間（秒）
			 */
			explicit PlayerStatus(float invincibleDuration);
			~PlayerStatus() override = default;


		private:
			/** ダメージ後の無敵時間 */
			float m_invincibleDuration;
			/** 無敵の残り時間 */
			float m_invincibleTimer;
		};
	}
}
