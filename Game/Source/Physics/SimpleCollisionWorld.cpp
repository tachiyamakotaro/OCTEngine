/**
 * @file SimpleCollisionWorld.cpp
 * @brief 矩形のリストで地形を表す、いちばんシンプルな当たり判定ワールド
 */
#include "stdafx.h"
#include "SimpleCollisionWorld.h"


namespace app
{
	namespace physics
	{
		void SimpleCollisionWorld::AddBlock(const float x, const float y, const float width, const float height)
		{
			AddSolid(math::Rect::FromMinSize(x, y, width, height));
		}


		void SimpleCollisionWorld::AddBlockFromCenter(const math::Vec2& center, const math::Vec2& size)
		{
			AddSolid(math::Rect::FromCenter(center, size));
		}


		void SimpleCollisionWorld::QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const
		{
			for (const math::Rect& solid : m_solids)
			{
				if (box.Overlaps(solid))
				{
					out.push_back(solid);
				}
			}
		}


		bool SimpleCollisionWorld::AnyOverlap(const math::Rect& box) const
		{
			for (const math::Rect& solid : m_solids)
			{
				if (box.Overlaps(solid))
				{
					return true;
				}
			}
			return false;
		}
	}
}
