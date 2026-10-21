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
			/** つかんだ角（崖の上面の、手前側の端） */
			math::Vec2 corner;
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

			// ---- 崖につかまっている間、つかんだ床と一緒に動く ----
			/**
			 * @brief つかんだ角が動く床なら、一緒に動くようにする（ぶら下がり・崖登りの間）
			 * @param corner つかんだ角（LedgeInfo::corner）
			 * @param facing 崖の方向（右なら 1、左なら -1）
			 */
			void BeginLedgeCarry(const math::Vec2& corner, int facing);
			/** @brief 一緒に動くのをやめる（運ばれた量 GetLedgeCarryOffset は、次に Begin するまで残る） */
			inline void EndLedgeCarry() { m_isLedgeCarry = false; }
			/** @brief BeginLedgeCarry してから、つかんだ床と一緒に動いた量 */
			inline const math::Vec2& GetLedgeCarryOffset() const { return m_ledgeCarryOffset; }
			/** @brief 床と一緒に動こうとしたら地形にぶつかって、崖から手が離れたか */
			inline bool IsLedgeLost() const { return m_isLedgeLost; }


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
			/** 足元のすぐ下の薄い箱（接地の判定と、乗っている動く床を探すのに使う） */
			math::Rect GetGroundProbe() const;
			/** つかんだ角のすぐ内側の小さな箱（つかんでいる動く床を探すのに使う） */
			math::Rect GetLedgeProbe() const;
			/** つかんだ角が動く床なら、動いた分だけ一緒に動く */
			void CarryWithLedge();


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

			/** 崖につかまって、つかんだ床と一緒に動いているか */
			bool m_isLedgeCarry;
			/** 床と一緒に動けずに、崖から手が離れたか */
			bool m_isLedgeLost;
			/** つかんだ角（床と一緒に動かしていく） */
			math::Vec2 m_ledgeCorner;
			/** 崖の方向（右なら 1、左なら -1） */
			int m_ledgeFacing;
			/** BeginLedgeCarry してから、一緒に動いた量 */
			math::Vec2 m_ledgeCarryOffset;
		};
	}
}
