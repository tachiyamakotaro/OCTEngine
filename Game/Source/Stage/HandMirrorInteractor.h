/**
 * @file HandMirrorInteractor.h
 * @brief プレイヤーによる手鏡の操作（構える・傾ける）
 * @details 拾った手鏡（HandMirror）に、毎フレーム「構えているか・手元の位置・向き・傾き」を入れる。
 *          入力はキャラクターのコマンド（CharacterCommand の handMirrorHold / handMirrorAim）を見る。
 *          DebugScene がプレイヤーの更新のあとに呼ぶ（反射鏡の MirrorInteractor と同じ）。
 */
#pragma once


namespace app
{
	namespace actor
	{
		class CharacterBase;
	}


	namespace stage
	{
		class Stage;


		/**
		 * @brief 手鏡の操作
		 */
		class HandMirrorInteractor
		{
		public:
			/** 手鏡の位置：体の中心から、向いている方へどれだけ前か */
			float handOffsetX = 35.0f;
			/** 手鏡の高さ：体の高さに対する割合（0 = 足元、1 = 頭） */
			float handHeightRate = 0.55f;


		public:
			/**
			 * @brief 毎フレーム（プレイヤーの更新のあと）
			 * @param stage ステージ（拾った手鏡を探す）
			 * @param character 手鏡を持つキャラクター
			 * @param deltaTime 経過時間（傾ける速さに使う）
			 */
			void Update(Stage& stage, const actor::CharacterBase& character, float deltaTime);
			/** @brief 最初の状態に戻す */
			void Reset();
			/** @brief 手鏡を持っているか（プレイヤーの入力の切り替えに使う） */
			bool HasHandMirror() const { return m_hasHandMirror; }
			/** @brief 構えているか */
			bool IsRaised() const { return m_isRaised; }


		private:
			/** 手鏡を持っているか */
			bool m_hasHandMirror = false;
			/** 構えているか */
			bool m_isRaised = false;
		};
	}
}
