/**
 * @file StageBlock.cpp
 * @brief 固い地形ブロック
 */
#include "stdafx.h"
#include "StageBlock.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_BLOCK = IM_COL32(120, 125, 140, 255);
		}


		StageBlock::StageBlock(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		CollisionKind StageBlock::GetCollision(math::Rect& outRect) const
		{
			outRect = rect;
			return CollisionKind::Terrain;
		}


		bool StageBlock::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (!RayIntersectRect(origin, direction, rect, outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::Stop;
			return true;
		}


		void StageBlock::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			canvas.Rect(rect, EDITOR_COLOR_BLOCK, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
		}
	}
}
