/**
 * @file Grass.cpp
 * @brief 草むら
 */
#include "stdafx.h"
#include "Grass.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		Grass::Grass(const char* name, const float x, const float y, const float width, const float height)
			: BurnableGimmick(name, x, y, width, height)
		{
			igniteTime = 1.0f;
			burnTime = 1.5f;
			spreadTime = 0.35f;
		}


		CollisionKind Grass::GetCollision(math::Rect& outRect) const
		{
			if (isBurnt)
			{
				return CollisionKind::None;
			}
			outRect = GetCurrentRect();
			return CollisionKind::Terrain;
		}


		bool Grass::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			// 燃え尽きるまでは光を止める（ふつうの光では燃えない）
			if (isBurnt || !RayIntersectRect(origin, direction, GetCurrentRect(), outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::Stop;
			return true;
		}


		math::Rect Grass::GetCurrentRect() const
		{
			if (!isBurning)
			{
				return rect;
			}
			// 燃え始めはゆっくり、だんだん速く縮む
			const float scale = 1.0f - math::SmoothStep(GetBurnProgress());
			const math::Vec2 center = (rect.min + rect.max) * 0.5f;
			return math::Rect::FromCenter(center, math::Vec2(rect.Width() * scale, rect.Height() * scale));
		}


		unsigned int Grass::GetEditorColor() const
		{
			return IM_COL32(70, 160, 60, 255);
		}
	}
}
