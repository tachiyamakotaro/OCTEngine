/**
 * @file StartEditor.cpp
 * @brief ステージ開始の演出を調整する ImGui
 */
#include "stdafx.h"
#include "StartEditor.h"

#include "StartSequence.h"
#include "imgui.h"


namespace app
{
	namespace stage
	{
		void StartEditor::Draw(StartSequence& sequence)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("StartEditor");

			StartTiming& timing = sequence.GetTimingForEdit();
			ImGui::SeparatorText("Start Timing");
			ImGui::SliderFloat("Start Circle", &timing.startCloseRate, 0.0f, 1.0f, "%.2f");
			ImGui::SetItemTooltip("How closed the circle is at first.\n0 = fully open, 1 = black. Bigger = smaller circle.");
			ImGui::SliderFloat("Hold", &timing.holdTime, 0.0f, 3.0f, "%.2f s");
			ImGui::SetItemTooltip("Wait with the small circle (no control) after the screen fade.");
			ImGui::SliderFloat("Circle Open", &timing.irisOpenTime, 0.1f, 3.0f, "%.2f s");
			ImGui::SetItemTooltip("Time for the circle to open.\nCamera zoom-out speed is Event Camera > Stage Start > Blend Out.");

			ImGui::Separator();
			ImGui::Text("%s", sequence.IsHolding() ? "HOLDING" : (sequence.IsPlaying() ? "OPENING" : "-"));
			if (ImGui::Button("Reset Start"))
			{
				sequence.ResetTiming();
			}
			ImGui::SetItemTooltip("Back to the values given in Init.");

			ImGui::PopID();
		}
	}
}
