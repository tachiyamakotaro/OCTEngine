/**
 * @file LightReceiver.cpp
 * @brief 受光器
 */
#include "stdafx.h"
#include "LightReceiver.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_OFF = IM_COL32(150, 140, 60, 255);
			const ImU32 EDITOR_COLOR_ON = IM_COL32(255, 235, 90, 255);
		}


		LightReceiver::LightReceiver(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void LightReceiver::Update(const GimmickUpdateContext& context)
		{
			isLit = context.light.IsLit(*this);
		}


		bool LightReceiver::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (!RayIntersectRect(origin, direction, rect, outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::Absorb;
			return true;
		}


		void LightReceiver::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			canvas.Rect(rect, isLit ? EDITOR_COLOR_ON : EDITOR_COLOR_OFF, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void LightReceiver::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::Text("Lit: %s", isLit ? "YES" : "no");
		}
	}
}
