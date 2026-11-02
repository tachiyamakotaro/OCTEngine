/**
 * @file GimmickMarkEditor.cpp
 * @brief ギミックの印（電球マーク・歯車・レール）を調整する ImGui
 */
#include "stdafx.h"
#include "GimmickMarkEditor.h"

#include "GimmickMarkRenderer.h"
#include "imgui.h"


namespace
{
	/**
	 * @brief 印の色1つぶんの項目（色と光る強さ）
	 * @param label 項目の名前（色は label、光る強さは「label Glow」）
	 * @param maxGlow 光る強さの上限
	 */
	void DrawMarkColor(const char* label, app::stage::MarkColor& color, const float maxGlow)
	{
		ImGui::PushID(label);
		ImGui::ColorEdit3(label, color.rgb);
		if (maxGlow > 0.0f)
		{
			ImGui::SliderFloat("Glow", &color.glow, 0.0f, maxGlow, "%.2f");
			ImGui::SetItemTooltip("Self glow. Over 1 = bloom.");
		}
		ImGui::PopID();
	}
}


namespace app
{
	namespace stage
	{
		void GimmickMarkEditor::Draw(GimmickMarkRenderer& renderer)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("GimmickMarkEditor");

			GimmickMarkParameter& param = renderer.GetParameterForEdit();
			DrawBulb(param.bulb);
			DrawGear(param.gear);
			DrawRail(param.rail);
			DrawPlacement(param.placement);

			ImGui::Separator();
			ImGui::Text("Marks: %d", static_cast<int>(renderer.GetCount()));
			if (ImGui::Button("Reset Marks"))
			{
				renderer.ResetParameter();
			}
			ImGui::SetItemTooltip("Back to the values given in Init.");

			ImGui::PopID();
		}


		void GimmickMarkEditor::DrawBulb(BulbStyle& style)
		{
			ImGui::SeparatorText("Receiver Bulb");
			ImGui::PushID("Bulb");
			ImGui::Checkbox("Visible", &style.isVisible);
			ImGui::SliderFloat("Scale", &style.scale, 0.1f, 1.5f, "%.2f");
			ImGui::SetItemTooltip("1 = the shorter side of the receiver.");
			// 光が当たっていないとき（白っぽくして見やすくする）
			DrawMarkColor("Off Color", style.off, 2.0f);
			DrawMarkColor("Lit Color", style.lit, 5.0f);
			DrawMarkColor("Outline", style.outline, 0.0f);
			ImGui::PopID();
		}


		void GimmickMarkEditor::DrawGear(GearStyle& style)
		{
			ImGui::SeparatorText("Mirror Gear");
			ImGui::PushID("Gear");
			ImGui::Checkbox("Visible", &style.isVisible);
			ImGui::SetItemTooltip("Only for mirrors with Rotatable ON.");
			ImGui::DragFloat("Size", &style.size, 0.5f, 1.0f, 300.0f, "%.1f");
			ImGui::DragFloat("Teeth", &style.teeth, 1.0f, 3.0f, 30.0f, "%.0f");
			ImGui::DragFloat("Thickness", &style.thickness, 0.5f, 0.5f, 100.0f, "%.1f");
			DrawMarkColor("Body", style.body, 1.0f);
			DrawMarkColor("Dark", style.dark, 0.0f);
			ImGui::PopID();
		}


		void GimmickMarkEditor::DrawRail(RailStyle& style)
		{
			ImGui::SeparatorText("Mirror Rail");
			ImGui::PushID("Rail");
			ImGui::Checkbox("Visible", &style.isVisible);
			ImGui::SetItemTooltip("Only for mirrors with Movable + Rail ON (Rail Min X < Rail Max X).");
			ImGui::DragFloat("Height", &style.height, 0.5f, 1.0f, 200.0f, "%.1f");
			ImGui::DragFloat("Depth", &style.depth, 0.5f, 1.0f, 200.0f, "%.1f");
			ImGui::DragFloat("Extend", &style.extend, 0.5f, 0.0f, 500.0f, "%.1f");
			ImGui::SetItemTooltip("Extra length on both ends. 0 = exactly the move range.");
			ImGui::DragFloat("Tie Spacing", &style.tieSpacing, 0.5f, 1.0f, 500.0f, "%.1f");
			DrawMarkColor("Body", style.body, 1.0f);
			DrawMarkColor("Dark", style.dark, 0.0f);
			ImGui::DragFloat("Stopper Width", &style.stopperWidth, 0.5f, 0.0f, 200.0f, "%.1f");
			ImGui::DragFloat("Stopper Height", &style.stopperHeight, 0.5f, 0.0f, 200.0f, "%.1f");
			DrawMarkColor("Stopper", style.stopper, 1.0f);
			ImGui::PopID();
		}


		void GimmickMarkEditor::DrawPlacement(MarkPlacement& placement)
		{
			ImGui::SeparatorText("Placement");
			ImGui::PushID("Placement");
			ImGui::DragFloat("Receiver Depth", &placement.receiverDepth, 1.0f, 1.0f, 1000.0f, "%.0f");
			ImGui::SetItemTooltip("Same as GimmickRenderer receiverDepth. The bulb sits on the front face.");
			ImGui::DragFloat("Mirror Depth", &placement.lightMirrorDepth, 1.0f, 1.0f, 1000.0f, "%.0f");
			ImGui::SetItemTooltip("Same as GimmickRenderer lightMirrorDepth. Gear sits in front of the mirror, rail behind it.");
			ImGui::DragFloat("Surface Gap", &placement.surfaceGap, 0.1f, 0.0f, 50.0f, "%.1f");
			ImGui::PopID();
		}
	}
}
