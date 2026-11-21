/**
 * @file Rect.h
 * @brief 軸に平行な矩形（当たり判定用）
 * @details エンジンの nsK2EngineLow::AABB（3D用）と名前が衝突しないよう Rect という名前にしている。
 */
#pragma once
#include "Vec2.h"


namespace app
{
	namespace math
	{
		/**
		 * @brief 軸に平行な矩形（min が左下、max が右上）
		 */
		struct Rect
		{
			/** 左下 */
			Vec2 min;
			/** 右上 */
			Vec2 max;


			/**
			 * @brief 足元中心の座標から矩形を作る（キャラクター用）
			 * @param foot 足元の中心座標
			 * @param width 幅
			 * @param height 高さ
			 */
			static Rect FromFoot(const Vec2& foot, const float width, const float height)
			{
				const float half = width * 0.5f;
				return { { foot.x - half, foot.y }, { foot.x + half, foot.y + height } };
			}
			/**
			 * @brief 中心座標とサイズから矩形を作る（レベルに配置したブロック用）
			 */
			static Rect FromCenter(const Vec2& center, const Vec2& size)
			{
				const Vec2 half = size * 0.5f;
				return { center - half, center + half };
			}
			/**
			 * @brief 左下座標とサイズから矩形を作る
			 */
			static Rect FromMinSize(const float x, const float y, const float width, const float height)
			{
				return { { x, y }, { x + width, y + height } };
			}


			float Width() const { return max.x - min.x; }
			float Height() const { return max.y - min.y; }
			/** 真ん中 */
			Vec2 Center() const { return (min + max) * 0.5f; }

			/**
			 * @brief まわりを margin だけ広げた矩形（上下左右それぞれ。マイナスなら広げない）
			 */
			Rect Expanded(const float margin) const
			{
				const float m = margin > 0.0f ? margin : 0.0f;
				return { { min.x - m, min.y - m }, { max.x + m, max.y + m } };
			}
			/**
			 * @brief この矩形と o の両方が入る矩形
			 */
			Rect Merged(const Rect& o) const
			{
				return {
					{ min.x < o.min.x ? min.x : o.min.x, min.y < o.min.y ? min.y : o.min.y },
					{ max.x > o.max.x ? max.x : o.max.x, max.y > o.max.y ? max.y : o.max.y } };
			}

			/**
			 * @brief 重なっているか
			 * @note ぴったり接しているだけの場合は重なりとみなさない
			 */
			bool Overlaps(const Rect& o) const
			{
				return min.x < o.max.x && max.x > o.min.x
					&& min.y < o.max.y && max.y > o.min.y;
			}
		};
	}
}
