/**
 * @file GoalEditor.cpp
 * @brief ゴールの門とクリアの演出を調整する ImGui
 */
#include "stdafx.h"
#include "GoalEditor.h"

#include "ClearSequence.h"
#include "GoalGateRenderer.h"
#include "imgui.h"
#include "Source/Core/IrisWipe.h"


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
		void GoalEditor::Draw(ClearSequence& sequence, GoalGateRenderer& renderer, core::IrisWipe& iris)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("GoalEditor");

			DrawTiming(sequence.GetTimingForEdit());
			DrawGate(renderer.GetStyleForEdit());
			DrawIris(iris.GetStyleForEdit());

			ImGui::Separator();
			ImGui::Text("Gates: %d   %s", static_cast<int>(renderer.GetCount()), sequence.IsStarted() ? "CLEARING" : "");
			if (ImGui::Button("Reset Goal"))
			{
				sequence.ResetTiming();
				renderer.ResetStyle();
				iris.ResetStyle();
			}
			ImGui::SetItemTooltip("Back to the values given in Init.");

			ImGui::PopID();
		}


		void GoalEditor::DrawTiming(ClearTiming& timing)
		{
			ImGui::SeparatorText("Clear Timing");
			ImGui::PushID("Timing");
			ImGui::SliderFloat("Door Open", &timing.doorOpenTime, 0.0f, 3.0f, "%.2f s");
			ImGui::SliderFloat("Wait", &timing.waitAfterOpen, 0.0f, 2.0f, "%.2f s");
			ImGui::SetItemTooltip("From the door fully open to the circle starting to close.");
			ImGui::SliderFloat("Circle Close", &timing.irisCloseTime, 0.1f, 3.0f, "%.2f s");
			ImGui::SliderFloat("Black Hold", &timing.blackHoldTime, 0.0f, 2.0f, "%.2f s");
			ImGui::SetItemTooltip("Stay black for this time, then go to the next stage.");
			ImGui::Text("Total: %.2f s", timing.GetTotalTime());
			ImGui::PopID();
		}


		void GoalEditor::DrawGate(GateStyle& style)
		{
			ImGui::SeparatorText("Gate");
			ImGui::PushID("Gate");
			ImGui::SliderFloat("Open Angle", &style.doorOpenAngleDeg, -120.0f, 120.0f, "%.0f deg");
			ImGui::SetItemTooltip("The doors open to the back. If they open to the front, make this negative.");
			ImGui::SliderFloat("Preview Open", &style.previewOpenRate, 0.0f, 1.0f, "%.2f");
			ImGui::SetItemTooltip("Open every door by this rate without clearing (for checking). Set 0 to play.");
			DrawGlowColor("Light Closed", style.lightClosed);
			DrawGlowColor("Light Open", style.lightOpen);
			DrawGlowColor("Light Core", style.lightCore);
			ImGui::Checkbox("Torch Lit", &style.isTorchLit);
			DrawGlowColor("Flame Outer", style.flameOuter);
			DrawGlowColor("Flame Inner", style.flameInner);
			ImGui::SliderFloat("Flame Width", &style.flameWidth, 1.0f, 40.0f, "%.1f");
			ImGui::SliderFloat("Flame Height", &style.flameHeight, 1.0f, 80.0f, "%.1f");
			ImGui::SliderFloat("Flicker Speed", &style.flickerSpeed, 0.0f, 30.0f, "%.1f");
			ImGui::SliderFloat("Flicker Amount", &style.flickerAmount, 0.0f, 0.5f, "%.2f");
			ImGui::PopID();
		}


		void GoalEditor::DrawIris(core::IrisStyle& style)
		{
			ImGui::SeparatorText("Circle Fade");
			ImGui::PushID("Iris");
			ImGui::SliderFloat("Edge Softness", &style.edgeSoftness, 0.5f, 40.0f, "%.1f px");
			ImGui::SliderFloat("Focus Offset Y", &style.focusOffsetY, -200.0f, 200.0f, "%.0f");
			ImGui::SetItemTooltip("Move the center of the circle up from the middle of the player.");
			ImGui::PopID();
		}
	}
}
