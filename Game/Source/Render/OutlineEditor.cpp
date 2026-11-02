/**
 * @file OutlineEditor.cpp
 * @brief アウトライン（黒い縁）を調整する ImGui
 */
#include "stdafx.h"
#include "OutlineEditor.h"

#include "imgui.h"


namespace app
{
	namespace render
	{
		void OutlineEditor::Draw(RenderingEngine& renderingEngine)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("OutlineEditor");

			RenderingEngine::OutlineParam& outline = renderingEngine.GetOutlineParam();
			ImGui::Checkbox("Enable Outline", &renderingEngine.IsEnableOutline());
			ImGui::ColorEdit3("Line Color", outline.color);
			ImGui::SliderFloat("Line Strength", &outline.color[3], 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Thickness", &outline.thickness, 0.5f, 6.0f, "%.1f px");
			ImGui::SliderFloat("Depth Threshold", &outline.depthThreshold, 1.0f, 500.0f, "%.0f");
			ImGui::SetItemTooltip("Draw a line where the next pixel is this much farther from the camera. Smaller = more lines.");

			ImGui::PopID();
		}
	}
}
