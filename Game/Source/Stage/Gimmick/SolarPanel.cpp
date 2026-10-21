/**
 * @file SolarPanel.cpp
 * @brief ソーラーパネル
 */
#include "stdafx.h"
#include "SolarPanel.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/** 光が当たっていないとき（暗い青） */
			const ImU32 EDITOR_COLOR_OFF = IM_COL32(40, 60, 120, 255);
			/** 光が当たっているとき（明るい青） */
			const ImU32 EDITOR_COLOR_ON = IM_COL32(110, 180, 255, 255);
		}


		SolarPanel::SolarPanel(const char* name, const float x, const float y, const float width, const float height)
			: LightReceiver(name, x, y, width, height)
		{}


		void SolarPanel::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			canvas.Rect(rect, isLit ? EDITOR_COLOR_ON : EDITOR_COLOR_OFF, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}
	}
}
