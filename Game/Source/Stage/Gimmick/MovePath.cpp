/**
 * @file MovePath.cpp
 * @brief 「始点と終点の間を動く」動きの部品
 */
#include "stdafx.h"
#include "MovePath.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		void MovePath::MoveToward(const float target, const float deltaTime)
		{
			const float length = GetLength();
			if (length <= 0.0f || speed <= 0.0f)
			{
				return;
			}
			// progress は 0〜1 なので、動く距離を道のりの長さで割って近づける
			progress = math::MoveTowards(progress, math::Clamp01(target), speed * deltaTime / length);
		}


		void MovePath::Rotate90(const int quarterTurns)
		{
			// +1 で反時計回りに90度：(x, y) → (-y, x)
			const int turns = ((quarterTurns % 4) + 4) % 4;
			for (int i = 0; i < turns; ++i)
			{
				offset = math::Vec2(-offset.y, offset.x);
			}
		}


		void MovePath::DrawInspector()
		{
			ImGui::SeparatorText("Move");
			float values[2] = { offset.x, offset.y };
			if (ImGui::DragFloat2("Move Offset", values, 1.0f, 0.0f, 0.0f, "%.1f"))
			{
				offset = math::Vec2(values[0], values[1]);
			}
			ImGui::SetItemTooltip("How far the end point is from the placed position.");
			ImGui::DragFloat("Move Speed", &speed, 1.0f, 1.0f, 5000.0f, "%.0f");
			ImGui::Text("Progress: %.0f%%", progress * 100.0f);
		}


		void MovePath::Serialize(GimmickArchive& archive)
		{
			archive.Field("moveOffset", offset);
			archive.Field("moveSpeed", speed);
		}


		void MovePath::DrawEditor(const EditorCanvas& canvas, const math::Vec2& startCenter, const unsigned int color) const
		{
			const math::Vec2 endCenter = startCenter + offset;
			canvas.Line(startCenter, endCenter, color, 1.5f);
			canvas.Dot(startCenter, 3.0f, color);
			canvas.Dot(endCenter, 4.0f, color);
		}
	}
}
