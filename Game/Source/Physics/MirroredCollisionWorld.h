/**
 * @file MirroredCollisionWorld.h
 * @brief 別の当たり判定ワールドを、鏡の面で左右反転して見せるワールド
 * @details 鏡の中の世界の地形は「こちらのステージを鏡に映したもの」なので、
 *          鏡の中の自分にはこれを渡す。問い合わせを反転して元のワールドに聞くだけなので、
 *          扉が開いた・光の足場が出た、といった元のステージの変化もそのまま鏡の中に映る。
 */
#pragma once
#include "ICollisionWorld.h"


namespace app
{
	namespace physics
	{
		/**
		 * @brief 鏡に映した当たり判定ワールド
		 */
		class MirroredCollisionWorld : public ICollisionWorld
		{
		public:
			/**
			 * @brief 元のワールドを設定
			 */
			inline void SetSource(const ICollisionWorld* source) { m_source = source; }
			/**
			 * @brief 鏡の面のX座標を設定
			 */
			inline void SetPlaneX(const float planeX) { m_planeX = planeX; }
			inline float GetPlaneX() const { return m_planeX; }

			/** @brief X座標を鏡に映す */
			inline float ReflectX(const float x) const { return 2.0f * m_planeX - x; }
			/** @brief 座標を鏡に映す */
			inline math::Vec2 Reflect(const math::Vec2& p) const { return { ReflectX(p.x), p.y }; }
			/** @brief 矩形を鏡に映す */
			math::Rect Reflect(const math::Rect& rect) const;


		public:
			void QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const override;
			bool AnyOverlap(const math::Rect& box) const override;


		public:
			MirroredCollisionWorld(const ICollisionWorld* source = nullptr, float planeX = 0.0f);


		private:
			/** 元のワールド */
			const ICollisionWorld* m_source;
			/** 鏡の面のX座標 */
			float m_planeX;
		};
	}
}
