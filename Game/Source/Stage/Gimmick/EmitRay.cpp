/**
 * @file EmitRay.cpp
 * @brief 光を出すギミック
 */
#include "stdafx.h"
#include "EmitRay.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/** エディタで描く円の半径（ピクセル） */
			constexpr float EDITOR_RADIUS_PX = 7.0f;
			/** 囲む矩形の半分の大きさ */
			constexpr float BOUNDS_HALF_SIZE = 15.0f;

			const ImU32 EDITOR_COLOR_SOURCE = IM_COL32(255, 160, 60, 255);
			const ImU32 EDITOR_COLOR_OFF = IM_COL32(90, 90, 100, 255);
		}


		EmitRay::EmitRay(const char* name, const float x, const float y, const float initialAngleDeg)
			: GimmickBase(name)
			, position(x, y)
			, angleDeg(initialAngleDeg)
		{}


		bool EmitRay::GetEmittedRay(math::Vec2& outOrigin, math::Vec2& outDirection) const
		{
			if (!isOn)
			{
				return false;
			}
			outOrigin = position;
			outDirection = GetDirection();
			return true;
		}


		math::Rect EmitRay::GetBounds() const
		{
			return math::Rect::FromMinSize(position.x - BOUNDS_HALF_SIZE, position.y - BOUNDS_HALF_SIZE, BOUNDS_HALF_SIZE * 2.0f, BOUNDS_HALF_SIZE * 2.0f);
		}


		bool EmitRay::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			// 円の半径ぶん、少し広めに拾う
			return (position - point).Length() <= pickDistance * 2.0f;
		}


		void EmitRay::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			const ImU32 color = EditorCanvas::Pick(isSelected, EDITOR_COLOR_SOURCE);
			canvas.Dot(position, EDITOR_RADIUS_PX, isOn ? color : EDITOR_COLOR_OFF);
			canvas.Line(position, position + GetDirection() * canvas.PixelsToWorld(20.0f), color, 2.0f);
			canvas.Label(position, name);
		}


		void EmitRay::DrawInspector(GimmickInspector& inspector)
		{
			ImGui::DragFloat2("Position", &position.x, 1.0f, 0.0f, 0.0f, "%.1f");
			inspector.EditAngle("Angle", angleDeg, 45.0f);
			ImGui::Checkbox("On", &isOn);
			ImGui::Checkbox("Wide Beam", &isWideBeam);
			ImGui::SetItemTooltip("ON: a thick beam. With a Band fake wall, the wall disappears as wide as the beam.");
			if (isWideBeam)
			{
				ImGui::DragFloat("Wide Beam Width", &wideBeamWidth, 1.0f, 1.0f, 1000.0f, "%.0f");
			}
		}


		void EmitRay::Serialize(GimmickArchive& archive)
		{
			archive.Field("position", position);
			archive.Field("angleDeg", angleDeg);
			archive.Field("isOn", isOn);
			archive.Field("isWideBeam", isWideBeam);
			archive.Field("wideBeamWidth", wideBeamWidth);
		}
	}
}
