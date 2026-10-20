/**
 * @file DarkZone.cpp
 * @brief 暗がり
 */
#include "stdafx.h"
#include "DarkZone.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_FILL = IM_COL32(10, 10, 25, 150);
			const ImU32 EDITOR_COLOR_LINE = IM_COL32(120, 110, 200, 255);
		}


		DarkZone::DarkZone(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		bool DarkZone::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			// 枠の線の近く（内側と外側に pickDistance）だけ当たり
			const bool isInOuter = point.x >= rect.min.x - pickDistance && point.x <= rect.max.x + pickDistance
				&& point.y >= rect.min.y - pickDistance && point.y <= rect.max.y + pickDistance;
			const bool isInInner = point.x > rect.min.x + pickDistance && point.x < rect.max.x - pickDistance
				&& point.y > rect.min.y + pickDistance && point.y < rect.max.y - pickDistance;
			return isInOuter && !isInInner;
		}


		void DarkZone::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			canvas.Rect(rect, EDITOR_COLOR_FILL, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE));
			canvas.Label({ rect.min.x, rect.max.y }, name + " (dark)");
		}


		void DarkZone::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::SliderFloat("Darkness", &darkness, 0.0f, 1.0f, "%.2f");
			ImGui::DragFloat("Light Radius", &lightRadius, 1.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::SetItemTooltip("Distance from a light beam that becomes bright.\nHidden floors inside this zone appear within this distance.");
			ImGui::DragFloat("Player Light", &playerLightRadius, 1.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::SetItemTooltip("Radius of the circle around the player that is always visible.");
			ImGui::SliderFloat("Player Light Soft", &playerLightSoftness, 0.0f, 1.0f, "%.2f");
			ImGui::SetItemTooltip("0 = a sharp edge. 1 = darker little by little from the center.");
			ImGui::Checkbox("Player Reveals Floors", &isPlayerRevealingFloors);
			ImGui::SetItemTooltip("ON: hidden floors inside the player's circle also appear (like Mario's dark levels).\nOFF: only light beams reveal hidden floors.");
			ImGui::DragFloat("Edge Feather", &edgeFeather, 1.0f, 0.0f, 1000.0f, "%.0f");
			ImGui::TextDisabled("Click the frame line to select this zone.");
		}


		void DarkZone::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("darkness", darkness);
			archive.Field("lightRadius", lightRadius);
			archive.Field("playerLightRadius", playerLightRadius);
			archive.Field("playerLightSoftness", playerLightSoftness);
			archive.Field("isPlayerRevealingFloors", isPlayerRevealingFloors);
			archive.Field("edgeFeather", edgeFeather);
		}
	}
}
