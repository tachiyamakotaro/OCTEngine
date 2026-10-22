/**
 * @file ShadeBoard.cpp
 * @brief 日よけ板
 */
#include "stdafx.h"
#include "ShadeBoard.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			// 木の板の色（モデルの色と合わせている）
			const ImU32 EDITOR_COLOR_FILL = IM_COL32(150, 100, 55, 230);
			const ImU32 EDITOR_COLOR_LINE = IM_COL32(210, 165, 110, 255);
			const ImU32 EDITOR_COLOR_RAIL = IM_COL32(210, 165, 110, 110);
		}


		ShadeBoard::ShadeBoard(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		bool ShadeBoard::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (!RayIntersectRect(origin, direction, rect, outHit.distance))
			{
				return false;
			}
			// 光をさえぎる（板の後ろが日陰になる）
			outHit.response = LightResponse::Stop;
			return true;
		}


		void ShadeBoard::SetAnchor(const math::Vec2& anchor)
		{
			const float dx = anchor.x - rect.min.x;
			RectGimmick::SetAnchor(anchor);
			railMinX += dx;
			railMaxX += dx;
		}


		void ShadeBoard::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			const float centerY = (rect.min.y + rect.max.y) * 0.5f;
			if (hasRail)
			{
				canvas.Line({ railMinX, centerY }, { railMaxX, centerY }, EDITOR_COLOR_RAIL, 1.0f);
			}
			canvas.Rect(rect, EDITOR_COLOR_FILL, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE));
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void ShadeBoard::DrawInspector(GimmickInspector& inspector)
		{
			// 位置の欄で動かしたときはレールはそのまま（反射鏡と同じ。見取り図でドラッグしたときはレールごと動く）
			RectGimmick::DrawInspector(inspector);
			ImGui::Checkbox("Movable", &isMovable);
			ImGui::Checkbox("Rail", &hasRail);
			if (hasRail)
			{
				ImGui::DragFloat("Rail Min X", &railMinX, 1.0f, 0.0f, 0.0f, "%.1f");
				ImGui::DragFloat("Rail Max X", &railMaxX, 1.0f, 0.0f, 0.0f, "%.1f");
				ImGui::SetItemTooltip("Range of the board's center X.");
				if (railMaxX < railMinX) { railMaxX = railMinX; }
			}
		}


		void ShadeBoard::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("isMovable", isMovable);
			archive.Field("hasRail", hasRail);
			archive.Field("railMinX", railMinX);
			archive.Field("railMaxX", railMaxX);
		}
	}
}
