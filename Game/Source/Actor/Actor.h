/**
 * @file Actor.h
 * @brief ステージに置かれる物（キャラクター・ギミックなど）の基底クラス
 * @details 横スクロールなので、位置は XY 平面の Vec2 で持つ。
 *          奥行き(Z)は描画するときにだけ使う（鏡の手前/奥のレイヤー分けなど）。
 */
#pragma once
#include "Source/Core/Math/Vec2.h"


namespace app
{
	namespace actor
	{
		/**
		 * @brief アクター基底クラス
		 */
		class Actor : public IGameObject
		{
		public:
			/**
			 * @brief 位置（XY平面）を取得
			 */
			inline const math::Vec2& GetPosition() const { return m_position; }
			/**
			 * @brief 位置（XY平面）を設定
			 * @note キャラクターの場合、Start() より前に呼ぶと初期位置になる
			 */
			inline void SetPosition(const math::Vec2& position) { m_position = position; }
			/**
			 * @brief 描画用の奥行き(Z)を取得
			 */
			inline float GetDepthZ() const { return m_depthZ; }
			/**
			 * @brief 描画用の奥行き(Z)を設定
			 */
			inline void SetDepthZ(const float depthZ) { m_depthZ = depthZ; }
			/**
			 * @brief 描画用の3D座標を取得
			 */
			inline Vector3 GetWorldPosition() const { return Vector3(m_position.x, m_position.y, m_depthZ); }


		public:
			Actor() = default;
			virtual ~Actor() override = default;


		protected:
			/** 位置（キャラクターは足元の中心） */
			math::Vec2 m_position;
			/** 描画用の奥行き */
			float m_depthZ = 0.0f;
		};
	}
}
