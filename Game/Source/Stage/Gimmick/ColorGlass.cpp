/**
 * @file ColorGlass.cpp
 * @brief 色ガラス（通った光の色が変わる）
 */
#include "stdafx.h"
#include "ColorGlass.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/** 見取り図の塗りの不透明度（0〜255。ガラスなので薄く） */
			constexpr uint32_t EDITOR_FILL_ALPHA = 90;
		}


		ColorGlass::ColorGlass(const char* name, const float x, const float y, const float width, const float height, const LightColor initialColor)
			: RectGimmick(name, x, y, width, height)
			, color(initialColor)
		{}


		void ColorGlass::Update(const GimmickUpdateContext& context)
		{
			incomingColor = context.light.GetLitColor(*this);
			isLit = incomingColor != LightColor::None;
		}


		void ColorGlass::ResetRuntimeState()
		{
			isLit = false;
			incomingColor = LightColor::None;
		}


		CollisionKind ColorGlass::GetCollision(math::Rect& outRect) const
		{
			if (!isSolid)
			{
				return CollisionKind::None;
			}
			outRect = rect;
			return CollisionKind::Terrain;
		}


		bool ColorGlass::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			// 光は通り抜けるが、ここから先はガラスの色の成分だけ残る
			if (!RayIntersectRect(origin, direction, rect, outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::Filter;
			outHit.filterColor = color;
			return true;
		}


		void ColorGlass::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// ガラスの色で薄く塗る（IM_COL32 の並びは 0xAABBGGRR なので、上の 8 ビットが不透明度）
			const uint32_t opaque = GetLightColorEditorColor(color);
			const ImU32 fill = (opaque & 0x00FFFFFFu) | (EDITOR_FILL_ALPHA << 24);
			canvas.Rect(rect, fill, EditorCanvas::Pick(isSelected, opaque));
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void ColorGlass::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			EditLightColor("Glass Color", color);
			ImGui::Checkbox("Solid (blocks player)", &isSolid);
			ImGui::Text("In: %s -> Out: %s", GetLightColorName(incomingColor), GetLightColorName(GetOutgoingColor()));
		}


		void ColorGlass::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			SerializeLightColor(archive, "color", color);
			archive.Field("isSolid", isSolid);
		}
	}
}
