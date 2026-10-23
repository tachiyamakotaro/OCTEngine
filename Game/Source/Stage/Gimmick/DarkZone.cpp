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


			/** 点と線分の距離 */
			float DistanceToSegment(const math::Vec2& p, const math::Vec2& a, const math::Vec2& b)
			{
				const math::Vec2 ab = b - a;
				const float lengthSq = math::Dot(ab, ab);
				const float t = lengthSq > 0.0f ? math::Clamp01(math::Dot(p - a, ab) / lengthSq) : 0.0f;
				return (p - (a + ab * t)).Length();
			}


			/** 斜めの暗がりを、見取り図で塗るときの横の帯の数 */
			constexpr int EDITOR_FILL_STRIPS = 24;
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
			if (point.y < rect.min.y || point.y > rect.max.y)
			{
				return false;
			}
			return point.x >= GetLeftX(point.y) && point.x <= GetRightX(point.y);
		}


		float DarkZone::GetLeftX(const float y) const
		{
			const float height = rect.Height();
			const float t = height > 0.0f ? math::Clamp01((y - rect.min.y) / height) : 0.0f;
			return rect.min.x + slantLeft * t;
		}


		float DarkZone::GetRightX(const float y) const
		{
			const float height = rect.Height();
			const float t = height > 0.0f ? math::Clamp01((y - rect.min.y) / height) : 0.0f;
			return rect.max.x + slantRight * t;
		}


		void DarkZone::GetCorners(math::Vec2 outCorners[4]) const
		{
			outCorners[0] = { rect.min.x, rect.min.y };
			outCorners[1] = { rect.max.x, rect.min.y };
			outCorners[2] = { rect.max.x + slantRight, rect.max.y };
			outCorners[3] = { rect.min.x + slantLeft, rect.max.y };
		}


		math::Rect DarkZone::GetDarkBounds() const
		{
			math::Rect bounds = rect;
			bounds.min.x = math::Min(rect.min.x, rect.min.x + slantLeft);
			bounds.max.x = math::Max(rect.max.x, rect.max.x + slantRight);
			return bounds;
		}


		bool DarkZone::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			// 端が斜めのときは、4本の辺のどれかの近くなら当たり
			if (IsSlanted())
			{
				math::Vec2 corners[4];
				GetCorners(corners);
				for (int i = 0; i < 4; ++i)
				{
					if (DistanceToSegment(point, corners[i], corners[(i + 1) % 4]) <= pickDistance)
					{
						return true;
					}
				}
				return false;
			}
			// 枠の線の近く（内側と外側に pickDistance）だけ当たり
			const bool isInOuter = point.x >= rect.min.x - pickDistance && point.x <= rect.max.x + pickDistance
				&& point.y >= rect.min.y - pickDistance && point.y <= rect.max.y + pickDistance;
			const bool isInInner = point.x > rect.min.x + pickDistance && point.x < rect.max.x - pickDistance
				&& point.y > rect.min.y + pickDistance && point.y < rect.max.y - pickDistance;
			return isInOuter && !isInInner;
		}


		void DarkZone::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 端が斜めのとき：横の帯を並べて塗り、4本の辺を線で描く
			if (!isWholeStage && IsSlanted())
			{
				const float height = rect.Height();
				for (int i = 0; i < EDITOR_FILL_STRIPS; ++i)
				{
					const float y0 = rect.min.y + height * static_cast<float>(i) / EDITOR_FILL_STRIPS;
					const float y1 = rect.min.y + height * static_cast<float>(i + 1) / EDITOR_FILL_STRIPS;
					const float yMid = (y0 + y1) * 0.5f;
					canvas.Rect({ { GetLeftX(yMid), y0 }, { GetRightX(yMid), y1 } }, EDITOR_COLOR_FILL, 0);
				}
				math::Vec2 corners[4];
				GetCorners(corners);
				const ImU32 lineColor = EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE);
				for (int i = 0; i < 4; ++i)
				{
					canvas.Line(corners[i], corners[(i + 1) % 4], lineColor);
				}
				canvas.Label(corners[3], name + " (dark)");
				return;
			}
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
				ImGui::DragFloat("Slant Left", &slantLeft, 1.0f, -5000.0f, 5000.0f, "%.0f");
				ImGui::SetItemTooltip("How far the TOP of the left edge is moved to the right from the bottom (minus = left).\n0 = a straight edge.");
				ImGui::DragFloat("Slant Right", &slantRight, 1.0f, -5000.0f, 5000.0f, "%.0f");
				ImGui::SetItemTooltip("How far the TOP of the right edge is moved to the right from the bottom (minus = left).\n0 = a straight edge.");
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
			archive.Field("slantLeft", slantLeft);
			archive.Field("slantRight", slantRight);
		}
	}
}
