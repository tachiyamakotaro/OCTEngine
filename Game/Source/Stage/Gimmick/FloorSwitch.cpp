/**
 * @file FloorSwitch.cpp
 * @brief 床スイッチ
 */
#include "stdafx.h"
#include "FloorSwitch.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_OFF = IM_COL32(230, 70, 60, 255);
			const ImU32 EDITOR_COLOR_ON = IM_COL32(90, 230, 110, 255);
		}


		FloorSwitch::FloorSwitch(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void FloorSwitch::Update(const GimmickUpdateContext& context)
		{
			bool isStepped = false;
			for (const math::Rect& body : context.bodies)
			{
				if (body.Overlaps(rect))
				{
					isStepped = true;
					break;
				}
			}
			// ラッチなら一度押したら押されたまま
			isPressed = isLatch ? (isPressed || isStepped) : isStepped;
		}


		bool FloorSwitch::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			return point.x >= rect.min.x && point.x <= rect.max.x
				&& point.y >= rect.min.y - pickDistance && point.y <= rect.max.y + pickDistance;
		}


		void FloorSwitch::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			canvas.Rect(rect, isPressed ? EDITOR_COLOR_ON : EDITOR_COLOR_OFF, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
			canvas.Label({ rect.min.x, rect.max.y + canvas.PixelsToWorld(20.0f) }, name);
		}


		void FloorSwitch::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::SetItemTooltip("Put it on top of a block. It has no collision.");
			ImGui::Checkbox("Latch (stay pressed)", &isLatch);
			ImGui::Text("Pressed: %s", isPressed ? "YES" : "no");
		}


		void FloorSwitch::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("isLatch", isLatch);
		}
	}
}
