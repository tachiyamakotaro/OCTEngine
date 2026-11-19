/**
 * @file BackgroundEditor.cpp
 * @brief ステージの奥の背景を調整する ImGui
 */
#include "stdafx.h"
#include "BackgroundEditor.h"

#include "BackgroundRenderer.h"
#include "imgui.h"


namespace
{
	/**
	 * @brief 光る色1つぶんの項目（色と光る強さ）
	 * @param label 項目の名前
	 */
	void DrawGlowColor(const char* label, app::stage::GlowColor& color)
	{
		ImGui::PushID(label);
		ImGui::ColorEdit3(label, color.rgb);
		ImGui::SliderFloat("Glow", &color.glow, 0.0f, 5.0f, "%.2f");
		ImGui::SetItemTooltip("Over 1 = bloom.");
		ImGui::PopID();
	}
}


namespace app
{
	namespace stage
	{
		void BackgroundEditor::Draw(BackgroundRenderer& renderer)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("BackgroundEditor");

			BackgroundStyle& style = renderer.GetStyleForEdit();
			ImGui::Checkbox("Visible", &style.isVisible);
			ImGui::SetItemTooltip("Off = everything behind the stage is the outside color.");
			ImGui::Text("Models: %d   Torch Lights: %d / %d",
				static_cast<int>(renderer.GetModelCount()), renderer.GetTorchLightCount(), BackgroundRenderer::MAX_TORCH_LIGHT);

			DrawCommon(style);
			DrawLayer("Near Columns", style.nearLayer, true);
			DrawTorch("Near Torches", style.nearTorch);
			DrawLayer("Far Columns", style.farLayer, true);
			DrawTorch("Far Torches", style.farTorch);
			DrawLayer("Back Wall", style.wall, false);

			ImGui::Separator();
			if (ImGui::Button("Reset Background"))
			{
				renderer.ResetStyle();
			}
			ImGui::SetItemTooltip("Back to the values given in Init.");

			ImGui::PopID();
		}


		void BackgroundEditor::DrawCommon(BackgroundStyle& style)
		{
			ImGui::SeparatorText("Common");
			ImGui::PushID("Common");
			ImGui::ColorEdit3("Fog Color", style.fogColor);
			ImGui::ColorEdit3("Outside Color", style.outsideColor);
			ImGui::SetItemTooltip("Color outside the stage (the rectangle around all blocks).");
			ImGui::SliderFloat("Stone Size", &style.stoneTextureWorldSize, 50.0f, 800.0f, "%.0f");
			ImGui::SliderFloat("Broken Rate", &style.brokenRate, 0.0f, 1.0f, "%.2f");
			ImGui::SetItemTooltip("Rate of bays with a collapsed arch.");
			ImGui::InputInt("Seed", &style.seed);
			ImGui::SetItemTooltip("Change to move the collapsed bays.");
			ImGui::PopID();
		}


		void BackgroundEditor::DrawLayer(const char* label, BackgroundLayerStyle& style, const bool hasScale)
		{
			ImGui::SeparatorText(label);
			ImGui::PushID(label);
			ImGui::Checkbox("Visible", &style.isVisible);
			ImGui::SliderFloat("Z", &style.z, 160.0f, 3000.0f, "%.0f");
			ImGui::SetItemTooltip("Depth of the front face. The back of the stage blocks is 150.");
			ImGui::SliderFloat("Parallax", &style.parallax, 0.0f, 0.95f, "%.2f");
			ImGui::SetItemTooltip("0 = moves with the stage, 1 = stuck to the screen. Higher looks farther.");
			if (hasScale)
			{
				ImGui::SliderFloat("Scale", &style.scale, 0.2f, 1.5f, "%.2f");
				ImGui::SetItemTooltip("Smaller looks farther.");
			}
			ImGui::ColorEdit3("Tint", style.tint);
			ImGui::SliderFloat("Ambient", &style.ambient, 0.0f, 1.5f, "%.2f");
			ImGui::SliderFloat("Sun Light", &style.sunLight, 0.0f, 1.5f, "%.2f");
			ImGui::SliderFloat("Fog", &style.fog, 0.0f, 1.0f, "%.2f");
			ImGui::SetItemTooltip("Higher = closer to the fog color (looks farther).");
			ImGui::PopID();
		}


		void BackgroundEditor::DrawTorch(const char* label, BackgroundTorchStyle& style)
		{
			ImGui::SeparatorText(label);
			ImGui::PushID(label);
			ImGui::Checkbox("Visible", &style.isVisible);
			ImGui::SliderInt("Every N Pillars", &style.pillarInterval, 0, 6);
			ImGui::SetItemTooltip("0 = no torches.");
			ImGui::SliderFloat("Height", &style.height, 0.0f, 800.0f, "%.0f");
			ImGui::ColorEdit3("Light Color", style.lightColor);
			ImGui::SliderFloat("Light Intensity", &style.lightIntensity, 0.0f, 6.0f, "%.2f");
			ImGui::SliderFloat("Light Range", &style.lightRange, 50.0f, 2000.0f, "%.0f");
			ImGui::SliderFloat("Flicker Speed", &style.flickerSpeed, 0.0f, 30.0f, "%.1f");
			ImGui::SliderFloat("Flicker Amount", &style.flickerAmount, 0.0f, 0.5f, "%.2f");
			DrawGlowColor("Flame Outer", style.flameOuter);
			DrawGlowColor("Flame Inner", style.flameInner);
			ImGui::SliderFloat("Flame Width", &style.flameWidth, 1.0f, 40.0f, "%.1f");
			ImGui::SliderFloat("Flame Height", &style.flameHeight, 1.0f, 80.0f, "%.1f");
			ImGui::PopID();
		}
	}
}
