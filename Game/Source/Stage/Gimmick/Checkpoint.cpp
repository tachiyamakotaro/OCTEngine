/**
 * @file Checkpoint.cpp
 * @brief チェックポイント
 */
#include "stdafx.h"
#include "Checkpoint.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_CHECKPOINT = IM_COL32(110, 230, 120, 160);
		}


		Checkpoint::Checkpoint(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void Checkpoint::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 範囲の枠と、再開位置の印
			canvas.Rect(rect, 0, EditorCanvas::Pick(isSelected, EDITOR_COLOR_CHECKPOINT));
			canvas.Dot(GetSpawnPosition(), 4.0f, EDITOR_COLOR_CHECKPOINT);
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void Checkpoint::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			const math::Vec2 spawn = GetSpawnPosition();
			ImGui::Text("Respawn at (%.0f, %.0f)", spawn.x, spawn.y);
			ImGui::TextWrapped("Touch this area to respawn here. Put the bottom on the floor.");
		}
	}
}
