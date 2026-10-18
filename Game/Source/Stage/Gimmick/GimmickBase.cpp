/**
 * @file GimmickBase.cpp
 * @brief ステージに置く物（ギミック）の基底クラス
 */
#include "stdafx.h"
#include "GimmickBase.h"


namespace app
{
	namespace stage
	{
		bool GimmickBase::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			// ふつうは全体を囲む矩形の中をクリックしたら当たり
			const math::Rect bounds = GetBounds();
			return point.x >= bounds.min.x && point.x <= bounds.max.x && point.y >= bounds.min.y && point.y <= bounds.max.y;
		}
	}
}
