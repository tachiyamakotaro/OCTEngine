/**
 * @file MirrorGlassEditor.cpp
 * @brief 奥の鏡の半透明ガラス（MirrorGlass）を調整する ImGui
 */
#include "stdafx.h"
#include "MirrorGlassEditor.h"

#include "MirrorGlass.h"
#include "imgui.h"


namespace app
{
	namespace render
	{
		void MirrorGlassEditor::Draw(MirrorGlass& glass)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("MirrorGlassEditor");

			MirrorGlass::Config& config = glass.GetConfig();
			ImGui::Checkbox("Visible", &config.isVisible);
			ImGui::SameLine();
			ImGui::TextDisabled(glass.IsDrawn() ? "(drawn)" : "(hidden)");
			ImGui::SliderFloat("Opacity", &config.opacity, 0.0f, 3.0f, "%.2f");
			ImGui::ColorEdit4("Tint", config.tint);
			ImGui::ColorEdit3("Edge Color", config.edgeColor, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
			ImGui::DragFloat("Edge Width", &config.edgeWidth, 5.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::ColorEdit3("Sheen Color", config.sheenColor);
			ImGui::SliderFloat("Sheen Opacity", &config.sheenOpacity, 0.0f, 1.0f, "%.2f");
			ImGui::DragFloat("Sheen Spacing", &config.sheenSpacing, 10.0f, 0.0f, 20000.0f, "%.0f");
			ImGui::DragFloat("Sheen Speed", &config.sheenSpeed, 0.005f, -2.0f, 2.0f, "%.3f");
			ImGui::DragFloat("Panel Size", &config.panelSize, 10.0f, 0.0f, 10000.0f, "%.0f");
			ImGui::SetItemTooltip("Size of one mirror panel (0 = no seams).");
			ImGui::DragFloat("Seam Width", &config.seamWidth, 0.1f, 0.0f, 100.0f, "%.1f");
			ImGui::SliderFloat("Seam Opacity", &config.seamOpacity, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Fresnel", &config.fresnel, 0.0f, 3.0f, "%.2f");
			ImGui::SliderFloat("Top Brightness", &config.topBrightness, 0.0f, 3.0f, "%.2f");
			ImGui::DragFloat("Z Offset", &config.zOffset, 0.1f, -100.0f, 100.0f, "%.1f");
			ImGui::SetItemTooltip("How far in front of the Back Mirror plane the glass sits.");

			ImGui::PopID();
		}
	}
}
