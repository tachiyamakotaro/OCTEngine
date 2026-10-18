/**
 * @file WarpZone.cpp
 * @brief ワープ
 */
#include "stdafx.h"
#include "WarpZone.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_WARP = IM_COL32(170, 120, 255, 255);
			const ImU32 EDITOR_COLOR_FILL = IM_COL32(60, 40, 110, 140);
		}


		WarpZone::WarpZone(
			const char* name, const float x, const float y, const float width, const float height, const float destX, const float destY)
			: RectGimmick(name, x, y, width, height)
			, destination(destX, destY)
		{}


		void WarpZone::RenameSignal(const std::string& oldName, const std::string& newName)
		{
			if (disabledBySignal == oldName) { disabledBySignal = newName; }
		}


		void WarpZone::RemoveSignal(const std::string& signalName)
		{
			if (disabledBySignal == signalName) { disabledBySignal.clear(); }
		}


		void WarpZone::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 範囲と、行き先までの線
			const ImU32 color = EditorCanvas::Pick(isSelected, EDITOR_COLOR_WARP);
			canvas.Rect(rect, EDITOR_COLOR_FILL, color);
			canvas.Line((rect.min + rect.max) * 0.5f, destination, color, isSelected ? 2.0f : 1.0f);
			canvas.Dot(destination, 4.0f, EDITOR_COLOR_WARP);
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void WarpZone::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::DragFloat2("Destination", &destination.x, 1.0f, 0.0f, 0.0f, "%.1f");
			ImGui::SetItemTooltip("Feet position after the warp. Keep it outside every warp zone.");
			inspector.SignalCombo("Disabled by", disabledBySignal);
			ImGui::SetItemTooltip("While this signal is ON, the warp does nothing.");
			ImGui::TextWrapped("The player warps when the center of the body enters this area. The running speed is kept.");
		}


		void WarpZone::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("destination", destination);
			archive.Field("disabledBySignal", disabledBySignal);
		}
	}
}
