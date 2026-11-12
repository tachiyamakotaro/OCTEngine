/**
 * @file BloomEditor.cpp
 * @brief ブルーム（明るい所から光があふれる効果）を調整する ImGui
 */
#include "stdafx.h"
#include "BloomEditor.h"

#include "imgui.h"


namespace app
{
	namespace render
	{
		void BloomEditor::Draw(RenderingEngine& renderingEngine)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("BloomEditor");

			ImGui::Checkbox("Enable Bloom", &renderingEngine.IsEnableBloom());
			ImGui::SetItemTooltip("OFF = skip the bloom passes (for comparing screenshots).");
			ImGui::SliderFloat("Threshold", &renderingEngine.GetBloomThreshold(), 0.0f, 3.0f, "%.2f");
			ImGui::SetItemTooltip("Only pixels brighter than this glow. 1 = only HDR (over 1.0) areas. Smaller = more glow.");
			ImGui::SliderFloat("Intensity", &renderingEngine.GetBloomIntensity(), 0.0f, 3.0f, "%.2f");
			ImGui::SetItemTooltip("How strongly the blurred light is added to the scene. 0 = no glow.");

			ImGui::PopID();
		}
	}
}
