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
			/** 折れ目の高さの範囲（上や下の辺にくっつくと、折れ目の片側の線が真横になってしまうので、少しあける） */
			constexpr float BEND_HEIGHT_MIN = 0.05f;
			constexpr float BEND_HEIGHT_MAX = 0.95f;


			/**
			 * @brief 折れ目の形（下の端 t=0 と上の端 t=1 で 0、折れ目の高さ t=h でちょうど 1 になる、まっすぐな線2本の山）
			 */
			float BendShape(const float t, const float h)
			{
				const float height = math::Clamp(h, BEND_HEIGHT_MIN, BEND_HEIGHT_MAX);
				return t <= height ? t / height : (1.0f - t) / (1.0f - height);
			}


			/**
			 * @brief 端の X（x = base + slant * t + bend * BendShape(t, h)）が、t = 0〜1 の間でいちばん小さく・大きくなる値
			 * @details まっすぐな線2本なので、下の端・折れ目・上の端の3か所のどれかがいちばん外側になる
			 */
			void GetEdgeRange(const float base, const float slant, const float bend, const float h, float& outMin, float& outMax)
			{
				const float height = math::Clamp(h, BEND_HEIGHT_MIN, BEND_HEIGHT_MAX);
				const float bottom = base;
				const float top = base + slant;
				const float kink = base + slant * height + bend;
				outMin = math::Min(math::Min(bottom, top), kink);
				outMax = math::Max(math::Max(bottom, top), kink);
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
			return rect.min.x + slantLeft * t + bendLeft * BendShape(t, bendLeftHeight);
		}


		float DarkZone::GetRightX(const float y) const
		{
			const float height = rect.Height();
			const float t = height > 0.0f ? math::Clamp01((y - rect.min.y) / height) : 0.0f;
			return rect.max.x + slantRight * t + bendRight * BendShape(t, bendRightHeight);
		}


		float DarkZone::GetBendY(const float bendHeight) const
		{
			return rect.min.y + rect.Height() * math::Clamp(bendHeight, BEND_HEIGHT_MIN, BEND_HEIGHT_MAX);
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
			// 左の端・右の端それぞれの、いちばん左といちばん右（曲がっている所も入れる）
			float leftMin = 0.0f;
			float leftMax = 0.0f;
			float rightMin = 0.0f;
			float rightMax = 0.0f;
			GetEdgeRange(rect.min.x, slantLeft, bendLeft, bendLeftHeight, leftMin, leftMax);
			GetEdgeRange(rect.max.x, slantRight, bendRight, bendRightHeight, rightMin, rightMax);
			math::Rect bounds = rect;
			bounds.min.x = math::Min(leftMin, rightMin);
			bounds.max.x = math::Max(leftMax, rightMax);
			return bounds;
		}


		void DarkZone::GetOutline(std::vector<math::Vec2>& out) const
		{
			out.clear();
			// 左右の端は「下の角 → 折れ目 → 上の角」のまっすぐな線2本
			// 下の辺（左 → 右）、右の端（下 → 折れ目 → 上）、上の辺（右 → 左）、左の端（上 → 折れ目 → 下）の順
			const float rightKinkY = GetBendY(bendRightHeight);
			const float leftKinkY = GetBendY(bendLeftHeight);
			out.push_back({ GetRightX(rect.min.y), rect.min.y });
			out.push_back({ GetRightX(rightKinkY), rightKinkY });
			out.push_back({ GetRightX(rect.max.y), rect.max.y });
			out.push_back({ GetLeftX(rect.max.y), rect.max.y });
			out.push_back({ GetLeftX(leftKinkY), leftKinkY });
			out.push_back({ GetLeftX(rect.min.y), rect.min.y });
		}


		math::Vec2 DarkZone::GetEditHandle(const int index) const
		{
			const float leftKinkY = GetBendY(bendLeftHeight);
			const float rightKinkY = GetBendY(bendRightHeight);
			switch (index)
			{
			case 0:		return { GetLeftX(leftKinkY), leftKinkY };
			case 1:		return { GetRightX(rightKinkY), rightKinkY };
			case 2:		return { rect.min.x + slantLeft, rect.max.y };
			case 3:		return { rect.max.x + slantRight, rect.max.y };
			default:	return math::Vec2::Zero();
			}
		}


		void DarkZone::SetEditHandle(const int index, const math::Vec2& position)
		{
			// 折れ目は上下左右に、角は横にだけ動かす
			const float height = rect.Height();
			const float t = height > 0.0f ? math::Clamp((position.y - rect.min.y) / height, BEND_HEIGHT_MIN, BEND_HEIGHT_MAX) : 0.5f;
			switch (index)
			{
			case 0:
				// 左の端の折れ目：高さが折れ目の高さ、まっすぐな端（その高さ）からの横のずれが bend
				bendLeftHeight = t;
				bendLeft = position.x - (rect.min.x + slantLeft * t);
				break;
			case 1:
				bendRightHeight = t;
				bendRight = position.x - (rect.max.x + slantRight * t);
				break;
			case 2:
			{
				// 左上の角：下の辺の左の端からのずれが傾き
				// 折れ目は今の場所から動かさない（入口の縁に合わせた折れ目が、角を動かしてもずれないように）
				const float kinkX = rect.min.x + slantLeft * bendLeftHeight + bendLeft;
				slantLeft = position.x - rect.min.x;
				bendLeft = kinkX - (rect.min.x + slantLeft * bendLeftHeight);
				break;
			}
			case 3:
			{
				const float kinkX = rect.max.x + slantRight * bendRightHeight + bendRight;
				slantRight = position.x - rect.max.x;
				bendRight = kinkX - (rect.max.x + slantRight * bendRightHeight);
				break;
			}
			default:
				break;
			}
		}


		bool DarkZone::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			// 端が斜め・曲がっているときは、まわりの線のどこかの近くなら当たり
			if (IsSlanted())
			{
				std::vector<math::Vec2> outline;
				GetOutline(outline);
				for (size_t i = 0; i < outline.size(); ++i)
				{
					if (DistanceToSegment(point, outline[i], outline[(i + 1) % outline.size()]) <= pickDistance)
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
			// 端が斜め・曲がっているとき：横の帯を並べて塗り、まわりを線で描く
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
				std::vector<math::Vec2> outline;
				GetOutline(outline);
				const ImU32 lineColor = EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE);
				for (size_t i = 0; i < outline.size(); ++i)
				{
					canvas.Line(outline[i], outline[(i + 1) % outline.size()], lineColor);
				}
				canvas.Label({ rect.min.x + slantLeft, rect.max.y }, name + " (dark)");
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
				ImGui::DragFloat("Bend Left", &bendLeft, 1.0f, -5000.0f, 5000.0f, "%.0f");
				ImGui::SetItemTooltip("Move the KINK of the left edge to the right (minus = left). The edge becomes two straight lines.\nMinus = the dark area grows to the left.");
				ImGui::SliderFloat("Bend Left Height", &bendLeftHeight, BEND_HEIGHT_MIN, BEND_HEIGHT_MAX, "%.2f");
				ImGui::SetItemTooltip("Height of the kink on the left edge (0 = bottom, 1 = top).");
				ImGui::DragFloat("Bend Right", &bendRight, 1.0f, -5000.0f, 5000.0f, "%.0f");
				ImGui::SetItemTooltip("Move the KINK of the right edge to the right (minus = left). The edge becomes two straight lines.\nPlus = the dark area grows to the right.");
				ImGui::SliderFloat("Bend Right Height", &bendRightHeight, BEND_HEIGHT_MIN, BEND_HEIGHT_MAX, "%.2f");
				ImGui::SetItemTooltip("Height of the kink on the right edge (0 = bottom, 1 = top).");
				ImGui::TextDisabled("Drag the square handles in the map to change slant / bend.");
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
			archive.Field("bendLeft", bendLeft);
			archive.Field("bendRight", bendRight);
			archive.Field("bendLeftHeight", bendLeftHeight);
			archive.Field("bendRightHeight", bendRightHeight);
		}
	}
}
