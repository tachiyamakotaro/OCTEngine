/**
 * @file ColorReceiver.cpp
 * @brief 色センサー
 */
#include "stdafx.h"
#include "ColorReceiver.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_BODY = IM_COL32(60, 60, 70, 255);
			const ImU32 EDITOR_COLOR_MATCHED = IM_COL32(255, 255, 255, 255);
			/** 見取り図で、センサーの色の窓を本体のどれくらいの大きさで描くか */
			constexpr float EDITOR_LENS_RATE = 0.6f;
		}


		ColorReceiver::ColorReceiver(const char* name, const float x, const float y, const float width, const float height, const LightColor initialColor)
			: RectGimmick(name, x, y, width, height)
			, requiredColor(initialColor)
		{}


		void ColorReceiver::Update(const GimmickUpdateContext& context)
		{
			// 当たっている光を全部重ねた色が、決められた色とぴったり同じなら点灯
			receivedColor = context.light.GetLitColor(*this);
			isMatched = receivedColor != LightColor::None && receivedColor == requiredColor;
		}


		void ColorReceiver::ResetRuntimeState()
		{
			receivedColor = LightColor::None;
			isMatched = false;
		}


		bool ColorReceiver::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			// 光はどの色でもここで止まり、当たった色が記録される
			if (!RayIntersectRect(origin, direction, rect, outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::Absorb;
			return true;
		}


		void ColorReceiver::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 本体（暗い箱）の真ん中に、センサーの色の窓。点灯したら白い枠
			canvas.Rect(rect, EDITOR_COLOR_BODY, isSelected ? EditorCanvas::COLOR_SELECTED : (isMatched ? EDITOR_COLOR_MATCHED : 0));
			const math::Vec2 center = (rect.min + rect.max) * 0.5f;
			const math::Vec2 half(rect.Width() * EDITOR_LENS_RATE * 0.5f, rect.Height() * EDITOR_LENS_RATE * 0.5f);
			canvas.Rect(math::Rect::FromCenter(center, half * 2.0f), GetLightColorEditorColor(requiredColor), 0);
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void ColorReceiver::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			EditLightColor("Required Color", requiredColor);
			ImGui::Text("Received: %s", GetLightColorName(receivedColor));
			ImGui::Text("Signal: %s", isMatched ? "ON" : "off");
		}


		void ColorReceiver::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			SerializeLightColor(archive, "requiredColor", requiredColor);
		}
	}
}
