/**
 * @file PlayerParameter.h
 * @brief プレイヤーのパラメーター（調整値）
 * @details キャラクター共通の値に、プレイヤーだけが使う値（ジャンプ・崖つかまり・落下ダメージ）を足す。
 */
#pragma once
#include "Source/Actor/Character/CharacterParameter.h"


namespace app
{
	namespace actor
	{
		/**
		 * @brief プレイヤーパラメーター
		 */
		struct PlayerParameter : public CharacterParameter
		{
			// ---- ジャンプ ----
			/** ジャンプの初速 */
			float jumpVelocity = 1100.0f;
			/** 上昇中にジャンプを離したときの上昇速度の倍率（小ジャンプ用。1.0で無効） */
			float jumpCutMultiplier = 0.5f;
			/** 足場から落ちた直後でもジャンプできる猶予（コヨーテタイム） */
			float coyoteTime = 0.1f;
			/** 着地前に押したジャンプを覚えておく時間（先行入力） */
			float jumpBufferTime = 0.12f;

			// ---- しゃがみ ----
			/** しゃがんだときの体の高さ */
			float crouchHeight = 70.0f;

			// ---- 崖つかまり（プリンス オブ ペルシャ）----
			/** true なら崖に自動でつかまる。false ならつかむボタンを押している間だけつかまる */
			bool isAutoLedgeGrab = true;
			/** 体の前面からどれだけ先の崖に手が届くか */
			float ledgeGrabReach = 25.0f;
			/** 頭より上、どこまでの崖をつかめるか */
			float ledgeGrabAbove = 40.0f;
			/** 頭より下、どこまでの崖をつかめるか */
			float ledgeGrabBelow = 40.0f;
			/** 崖から手を離したあと、再びつかめるようになるまでの時間 */
			float ledgeRegrabCooldown = 0.3f;
			/** 崖を登りきるまでの時間 */
			float climbUpDuration = 0.5f;

			// ---- 落下ダメージ（プリンス オブ ペルシャ：1段は平気、2段で痛い、3段で死ぬ）----
			/** この高さ以上から落ちると着地でよろけてダメージ */
			float hardLandingHeight = 350.0f;
			/** この高さ以上から落ちると死ぬ */
			float deadlyFallHeight = 700.0f;
			/** 落下ダメージ量 */
			int fallDamage = 1;
			/** 着地でよろけている時間 */
			float hardLandingDuration = 0.6f;

			// ---- 被ダメージ ----
			/** ダメージを受けたあとの無敵時間 */
			float invincibleTime = 1.0f;
		};
	}
}
