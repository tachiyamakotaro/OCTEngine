/**
 * @file DarknessEditor.cpp
 * @brief 暗がり（DarknessRenderer）の表示を切り替える ImGui
 */
#include "stdafx.h"
#include "DarknessEditor.h"

#include "DarknessRenderer.h"
#include "imgui.h"


namespace app
{
	namespace render
	{
		void DarknessEditor::Draw(DarknessRenderer& darkness)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("DarknessEditor");

			DarknessRenderer::Config& config = darkness.GetConfig();
			ImGui::Checkbox("Draw Dark Zones", &config.isDrawZones);
			ImGui::SetItemTooltip("OFF: hide the darkness (for screenshots). The stage JSON does not change.");
			ImGui::SameLine();
			ImGui::TextDisabled("(%d zone(s))", static_cast<int>(darkness.GetDrawCount()));
			ImGui::Checkbox("Draw Front Walls", &config.isDrawFrontWalls);
			ImGui::SetItemTooltip("OFF: hide the rock walls in front of dark rooms (for screenshots).");

			ImGui::PopID();
		}
	}
}
