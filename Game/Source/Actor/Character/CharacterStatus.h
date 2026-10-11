/**
 * @file CharacterStatus.h
 * @brief キャラクターのステータス基底クラス
 * @details ゲーム中に変化する値（HPなど）を持つ。調整値は CharacterParameter 側。
 */
#pragma once


namespace app
{
	namespace actor
	{
		struct CharacterParameter;


		/**
		 * @brief キャラクターのステータス基底クラス
		 */
		class CharacterStatus
		{
		public:
			/**
			 * @brief HPを取得
			 */
			inline int GetHp() const { return m_hp; }
			/**
			 * @brief 最大HPを取得
			 */
			inline int GetMaxHp() const { return m_maxHp; }
			/**
			 * @brief 死んでいるか
			 */
			inline bool IsDead() const { return m_hp <= 0; }


		public:
			/**
			 * @brief セットアップ
			 * @note ステータスの持ち主（CharacterBase）が Start 時に呼び出す
			 */
			virtual void Setup(const CharacterParameter& param);
			/**
			 * @brief 毎フレームの更新（タイマーなど）
			 */
			virtual void Update(float deltaTime);
			/**
			 * @brief ダメージを受ける
			 * @return 実際にダメージが入ったら true（無敵中などは false）
			 */
			virtual bool ApplyDamage(int damage);
			/**
			 * @brief 回復する
			 */
			void Heal(int amount);
			/**
			 * @brief 即死させる（落下死・トゲなど）
			 */
			void Kill();


		public:
			CharacterStatus();
			virtual ~CharacterStatus() = default;


		protected:
			/** HP */
			int m_hp;
			/** 最大HP */
			int m_maxHp;
		};
	}
}
