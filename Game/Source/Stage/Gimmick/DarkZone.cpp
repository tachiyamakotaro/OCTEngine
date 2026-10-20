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
			const ImU32 EDITOR_COLOR_FILL_WHOLE = IM_COL32(10, 10, 25, 60);
			const ImU32 EDITOR_COLOR_LINE = IM_COL32(120, 110, 200, 255);


			/** なめらかに 0 → 1（HLSL の smoothstep と同じ） */
			float SmoothStep(const float edge0, const float edge1, const float x)
			{
				if (edge1 <= edge0)
				{
					return x < edge0 ? 0.0f : 1.0f;
				}
				const float t = math::Clamp01((x - edge0) / (edge1 - edge0));
				return t * t * (3.0f - 2.0f * t);
			}
		}


		DarkZone::DarkZone(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		float DarkZone::Glow(const float distance, const float radius, const float softWidth)
		{
			if (radius <= 0.0f && softWidth <= 0.0f)
			{
				return 0.0f;
			}
			const float half = math::Max(softWidth, 0.0f) * 0.5f;
			return 1.0f - SmoothStep(radius - half, radius + half, distance);
		}


		bool DarkZone::Contains(const math::Vec2& point) const
		{
			if (isWholeStage)
			{
				return true;
			}
			return point.x >= rect.min.x && point.x <= rect.max.x && point.y >= rect.min.y && point.y <= rect.max.y;
		}


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
			// ステージ全体を暗くするときは、枠は選ぶためだけのものなので薄く描く
			canvas.Rect(rect, isWholeStage ? EDITOR_COLOR_FILL_WHOLE : EDITOR_COLOR_FILL, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE));
			canvas.Label({ rect.min.x, rect.max.y }, name + (isWholeStage ? " (dark: whole stage)" : " (dark)"));
		}


		void DarkZone::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::Checkbox("Whole Stage", &isWholeStage);
			ImGui::SetItemTooltip("ON: the whole stage is dark (the frame is only for selecting this zone).\nOFF: only inside the frame is dark.");
			ImGui::SliderFloat("Darkness", &darkness, 0.0f, 1.0f, "%.2f");
			ImGui::DragFloat("Light Radius", &lightRadius, 1.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::SetItemTooltip("Distance from a light beam to the middle of the bright/dark border.");
			ImGui::DragFloat("Light Soft Width", &lightSoftWidth, 1.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::SetItemTooltip("Width of the blurred border around light beams. 0 = a sharp border.");
			ImGui::DragFloat("Player Light", &playerLightRadius, 1.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::SetItemTooltip("Radius of the circle around the player that is always visible (middle of the border).");
			ImGui::DragFloat("Player Soft Width", &playerLightSoftWidth, 1.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::SetItemTooltip("Width of the blurred border of the player's circle. 0 = a sharp border.");
			ImGui::Checkbox("Player Reveals Floors", &isPlayerRevealingFloors);
			ImGui::SetItemTooltip("ON: hidden floors inside the player's circle also appear (like Mario's dark levels).\nOFF: only light beams reveal hidden floors.");
			if (!isWholeStage)
			{
				ImGui::DragFloat("Edge Feather", &edgeFeather, 1.0f, 0.0f, 1000.0f, "%.0f");
			}
			ImGui::TextDisabled("Click the frame line to select this zone.");
		}


		void DarkZone::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("isWholeStage", isWholeStage);
			archive.Field("darkness", darkness);
			archive.Field("lightRadius", lightRadius);
			archive.Field("lightSoftWidth", lightSoftWidth);
			archive.Field("playerLightRadius", playerLightRadius);
			archive.Field("playerLightSoftWidth", playerLightSoftWidth);
			archive.Field("isPlayerRevealingFloors", isPlayerRevealingFloors);
			archive.Field("edgeFeather", edgeFeather);
		}
	}
}
