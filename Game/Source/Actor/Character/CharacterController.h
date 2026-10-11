/**
 * @file CharacterController.h
 * @brief 横スクロール用キャラクターコントローラー（体の移動と地形との当たり判定）
 * @details 「速度を受け取って、地形にめり込まないように動かす」ことだけを担当する。
 *          どう動きたいか（走る・跳ぶ・つかまる）は各ステートが決めて速度に入れる。
 *
 *          エンジンの nsK2EngineLow::CharacterController（カプセル・3D用）は
 *          天井との当たり判定や崖の検出がないため、横スクロール用にこのクラスを用意している。
 *          名前は同じだが app::actor 名前空間にあるので、app::actor の中では自動でこちらが使われる。
 *
 *          座標は「足元の中心」。Y は上が正。
 */
#pragma once
#include <vector>
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace physics
	{
		class ICollisionWorld;
	}


	namespace actor
	{
		/**
		 * @brief 崖つかまり判定の条件
		 */
		struct LedgeQuery
		{
			/** 体の前面からどれだけ先まで手が届くか */
			float reach = 25.0f;
			/** 頭より上、どこまでの崖をつかめるか */
			float above = 40.0f;
			/** 頭より下、どこまでの崖をつかめるか */
			float below = 40.0f;
		};


		/**
		 * @brief 見つかった崖の情報
		 */
		struct LedgeInfo
		{
			/** ぶら下がるときの足元座標 */
			math::Vec2 hangPosition;
			/** 登りきったときの足元座標 */
			math::Vec2 standPosition;
			/** 崖の上面の高さ */
			float ledgeTopY = 0.0f;
			/** 崖の方向（右なら 1、左なら -1） */
			int facing = 1;
		};


		/**
		 * @brief 横スクロール用キャラクターコントローラー
		 */
		class CharacterController
		{
		public:
			/**
			 * @brief 初期化
			 * @param footPosition 足元の中心座標
			 * @param width 体の幅
			 * @param height 体の高さ
			 * @param world 地形の当たり判定（nullptr なら当たり判定なしで動く）
			 */
			void Init(const math::Vec2& footPosition, float width, float height, const physics::ICollisionWorld* world);
			/**
			 * @brief 速度に従って動かし、地形との当たり判定を解決する（毎フレーム1回）
			 */
			void Move(float deltaTime);


		public:
			// ---- 位置・速度 ----
			inline const math::Vec2& GetPosition() const { return m_position; }
			/** @brief 位置を直接設定する（崖へのスナップ・ワープ用。当たり判定はしない） */
			inline void SetPosition(const math::Vec2& position) { m_position = position; }
			inline const math::Vec2& GetVelocity() const { return m_velocity; }
			inline void SetVelocity(const math::Vec2& velocity) { m_velocity = velocity; }
			inline void SetVelocityX(const float vx) { m_velocity.x = vx; }
			inline void SetVelocityY(const float vy) { m_velocity.y = vy; }

			// ---- 重力 ----
			inline void SetGravity(const float gravity) { m_gravity = gravity; }
			inline void SetMaxFallSpeed(const float speed) { m_maxFallSpeed = speed; }
			/** @brief 重力を有効/無効にする（崖にぶら下がっている間など） */
			inline void SetGravityEnabled(const bool enabled) { m_isGravityEnabled = enabled; }
			inline bool IsGravityEnabled() const { return m_isGravityEnabled; }

			// ---- 接触状態（Move() の結果）----
			/** @brief 地面の上にいるか */
			inline bool IsOnGround() const { return m_isOnGround; }
			/** @brief このフレームで着地したか */
			inline bool IsJustLanded() const { return m_isJustLanded; }
			/** @brief このフレームで天井に頭をぶつけたか */
			inline bool IsHitCeiling() const { return m_isHitCeiling; }
			/**
			 * @brief 壁に接しているか
			 * @param direction 右なら 1、左なら -1
			 */
			inline bool IsTouchingWall(const int direction) const { return direction > 0 ? m_isTouchingWallRight : m_isTouchingWallLeft; }

			// ---- 体の大きさ ----
			inline float GetWidth() const { return m_width; }
			inline float GetHeight() const { return m_height; }
			/** @brief 現在の体の矩形 */
			math::Rect GetBounds() const;
			/** @brief 指定した足元座標での体の矩形 */
			math::Rect GetBoundsAt(const math::Vec2& footPosition) const;
			/**
			 * @brief 高さを変えられるか（しゃがみから立つときに頭上が空いているか）
			 */
			bool CanResizeHeight(float newHeight) const;
			/**
			 * @brief 足元を基準に高さを変える
			 * @return 変えられたら true
			 */
			bool TryResizeHeight(float newHeight);

			// ---- 地形への問い合わせ ----
			/** @brief 当たり判定ワールドを設定 */
			inline void SetCollisionWorld(const physics::ICollisionWorld* world) { m_collisionWorld = world; }
			/** @brief 矩形の範囲に地形がないか */
			bool IsBoxFree(const math::Rect& box) const;
			/**
			 * @brief 前方に足場が続いているか（エネミーの巡回で崖から落ちないようにする時など）
			 * @param direction 右なら 1、左なら -1
			 * @param distance 体の前面からどれだけ先を調べるか
			 * @param depth 足元からどれだけ下まで足場を探すか（段差の許容量）
			 */
			bool HasGroundAhead(int direction, float distance, float depth = 4.0f) const;
			/**
			 * @brief つかまれる崖を探す
			 * @param facing 探す向き（右なら 1、左なら -1）
			 * @param query 手の届く範囲
			 * @param out 見つかった崖の情報
			 * @return 見つかったら true
			 */
			bool FindLedge(int facing, const LedgeQuery& query, LedgeInfo& out) const;


		public:
			CharacterController();


		private:
			/** X方向に動かして横の当たり判定を解決 */
			void MoveAxisX(float dx);
			/** Y方向に動かして縦の当たり判定を解決 */
			void MoveAxisY(float dy);
			/**
			 * @brief 地形にめり込んでいたら押し出す
			 * @details 扉が閉じてきた・光の足場が体の位置に出た、など
			 *          地形の方が動いてめり込んだときのため。一番短い距離の方向へ押し出す。
			 */
			void ResolvePenetration();
			/** 接地・壁の状態を更新 */
			void UpdateContacts();


		private:
			/** 地形の当たり判定 */
			const physics::ICollisionWorld* m_collisionWorld;
			/** 判定結果の一時置き場（毎回メモリ確保しないため） */
			std::vector<math::Rect> m_hitBuffer;

			/** 足元の中心座標 */
			math::Vec2 m_position;
			/** 速度 */
			math::Vec2 m_velocity;
			/** 体の幅 */
			float m_width;
			/** 体の高さ */
			float m_height;

			/** 重力 */
			float m_gravity;
			/** 落下速度の上限 */
			float m_maxFallSpeed;
			/** 重力が有効か */
			bool m_isGravityEnabled;

			/** 地面の上にいるか */
			bool m_isOnGround;
			/** このフレームで着地したか */
			bool m_isJustLanded;
			/** このフレームで天井にぶつかったか */
			bool m_isHitCeiling;
			/** 左の壁に接しているか */
			bool m_isTouchingWallLeft;
			/** 右の壁に接しているか */
			bool m_isTouchingWallRight;
		};
	}
}
