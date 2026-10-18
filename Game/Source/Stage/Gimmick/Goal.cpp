/**
 * @file Goal.cpp
 * @brief ゴール
 */
#include "stdafx.h"
#include "Goal.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_GOAL = IM_COL32(255, 200, 40, 255);
		}


		Goal::Goal(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void Goal::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			canvas.Rect(rect, 0, EditorCanvas::Pick(isSelected, EDITOR_COLOR_GOAL));
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}
	}
}
