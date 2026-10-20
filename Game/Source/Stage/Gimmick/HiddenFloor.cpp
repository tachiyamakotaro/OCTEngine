/**
 * @file HiddenFloor.cpp
 * @brief 暗がりの足場
 */
#include "stdafx.h"
#include "HiddenFloor.h"
#include <cmath>
#include "DarkZone.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "GimmickManager.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_HIDDEN = IM_COL32(230, 210, 140, 110);
			const ImU32 EDITOR_COLOR_VISIBLE = IM_COL32(230, 210, 140, 220);
			const ImU32 EDITOR_COLOR_LIT = IM_COL32(255, 245, 160, 255);

			/** 時間の下限（0 で割らないように） */
			constexpr float MIN_TIME = 0.01f;


			/** 点と線分の距離 */
			float DistancePointToSegment(const math::Vec2& p, const math::Vec2& a, const math::Vec2& b)
			{
				const math::Vec2 ab = b - a;
				const float lengthSq = ab.LengthSq();
				const float t = lengthSq > 0.0f ? math::Clamp01(math::Dot(p - a, ab) / lengthSq) : 0.0f;
				return (p - (a + ab * t)).Length();
			}


			/** 点と矩形の距離（中なら 0） */
			float DistancePointToRect(const math::Vec2& p, const math::Rect& rect)
			{
				const float dx = math::Max(math::Max(rect.min.x - p.x, 0.0f), p.x - rect.max.x);
				const float dy = math::Max(math::Max(rect.min.y - p.y, 0.0f), p.y - rect.max.y);
				return std::sqrt(dx * dx + dy * dy);
			}


			/** 線分どうしが交わるか */
			bool SegmentsIntersect(const math::Vec2& a, const math::Vec2& b, const math::Vec2& c, const math::Vec2& d)
			{
				const float d1 = math::Cross(b - a, c - a);
				const float d2 = math::Cross(b - a, d - a);
				const float d3 = math::Cross(d - c, a - c);
				const float d4 = math::Cross(d - c, b - c);
				return ((d1 > 0.0f) != (d2 > 0.0f)) && ((d3 > 0.0f) != (d4 > 0.0f));
			}
		}


		float DistanceSegmentToRect(const math::Vec2& a, const math::Vec2& b, const math::Rect& rect)
		{
			// 端が矩形の中なら 0
			if (DistancePointToRect(a, rect) <= 0.0f || DistancePointToRect(b, rect) <= 0.0f)
			{
				return 0.0f;
			}
			const math::Vec2 corners[4] = { rect.min, { rect.max.x, rect.min.y }, rect.max, { rect.min.x, rect.max.y } };
			// 矩形の辺と交われば 0
			for (int i = 0; i < 4; ++i)
			{
				if (SegmentsIntersect(a, b, corners[i], corners[(i + 1) % 4]))
				{
					return 0.0f;
				}
			}
			// 交わらないなら、「線分の端と矩形」「矩形の角と線分」のうち一番近い距離
			float distance = math::Min(DistancePointToRect(a, rect), DistancePointToRect(b, rect));
			for (const math::Vec2& corner : corners)
			{
				distance = math::Min(distance, DistancePointToSegment(corner, a, b));
			}
			return distance;
		}


		HiddenFloor::HiddenFloor(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		const DarkZone* HiddenFloor::FindZone(const GimmickManager& gimmicks) const
		{
			const math::Vec2 center = (rect.min + rect.max) * 0.5f;
			for (const DarkZone& zone : gimmicks.View<DarkZone>())
			{
				const bool isInside = center.x >= zone.rect.min.x && center.x <= zone.rect.max.x
					&& center.y >= zone.rect.min.y && center.y <= zone.rect.max.y;
				if (isInside)
				{
					return &zone;
				}
			}
			return nullptr;
		}


		float HiddenFloor::GetRevealDistance(const GimmickManager& gimmicks) const
		{
			const DarkZone* zone = FindZone(gimmicks);
			return zone != nullptr ? zone->lightRadius : revealDistance;
		}


		void HiddenFloor::Update(const GimmickUpdateContext& context)
		{
			// 光の線分のどれかが、見える距離より近くを通っているか
			const DarkZone* zone = FindZone(context.gimmicks);
			const float distance = zone != nullptr ? zone->lightRadius : revealDistance;
			isLitNow = false;
			for (const LightSegment& segment : context.light.segments)
			{
				if (DistanceSegmentToRect(segment.start, segment.end, rect) <= distance)
				{
					isLitNow = true;
					break;
				}
			}
			// プレイヤーのまわりの丸の中に入っているか（暗がりで「プレイヤーのまわりでも見える」にしているときだけ）
			if (!isLitNow && zone != nullptr && zone->isPlayerRevealingFloors)
			{
				const float playerDistance = zone->GetPlayerRevealDistance();
				for (const math::Rect& body : context.bodies)
				{
					const math::Vec2 bodyCenter = (body.min + body.max) * 0.5f;
					if (DistanceSegmentToRect(bodyCenter, bodyCenter, rect) <= playerDistance)
					{
						isLitNow = true;
						break;
					}
				}
			}

			const bool wasVisible = IsVisible();
			if (isLitNow)
			{
				// 照らされている間：すぐに見えるようになり、見えたまま
				darkTimer = 0.0f;
				visibility = math::Min(visibility + context.deltaTime / math::Max(appearTime, MIN_TIME), 1.0f);
			}
			else if (visibility > 0.0f)
			{
				// 光が離れたら：stayTime 秒は見えたまま、そのあと fadeTime 秒で消える
				darkTimer += context.deltaTime;
				if (darkTimer > stayTime)
				{
					visibility = math::Max(visibility - context.deltaTime / math::Max(fadeTime, MIN_TIME), 0.0f);
				}
			}
			isRevealedThisFrame = !wasVisible && IsVisible();
		}


		void HiddenFloor::ResetRuntimeState()
		{
			visibility = 0.0f;
			isLitNow = false;
			darkTimer = 0.0f;
			isRevealedThisFrame = false;
		}


		CollisionKind HiddenFloor::GetCollision(math::Rect& outRect) const
		{
			if (!IsSolid())
			{
				return CollisionKind::None;
			}
			outRect = rect;
			return CollisionKind::Terrain;
		}


		void HiddenFloor::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 見えていなくても、エディタでは薄く描く（置いた場所が分かるように）
			const ImU32 fill = IsVisible() ? EDITOR_COLOR_VISIBLE : EDITOR_COLOR_HIDDEN;
			canvas.Rect(rect, fill, EditorCanvas::Pick(isSelected, isLitNow ? EDITOR_COLOR_LIT : EDITOR_COLOR_VISIBLE));
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void HiddenFloor::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::Checkbox("Solid when hidden", &isSolidWhenHidden);
			ImGui::SetItemTooltip("ON: you can stand on it even while it is hidden.\nOFF: you can stand on it only while it is visible.");
			ImGui::DragFloat("Stay Time", &stayTime, 0.05f, 0.0f, 30.0f, "%.2f s");
			ImGui::SetItemTooltip("Seconds it stays visible after the light leaves.");
			ImGui::DragFloat("Fade Time", &fadeTime, 0.05f, MIN_TIME, 10.0f, "%.2f s");
			ImGui::DragFloat("Appear Time", &appearTime, 0.01f, MIN_TIME, 5.0f, "%.2f s");
			ImGui::DragFloat("Reveal Distance", &revealDistance, 1.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::SetItemTooltip("Used only outside a Dark Zone. Inside a zone, the zone's Light Radius is used.");
			ImGui::Text("Lit: %s   Visible: %.2f", isLitNow ? "YES" : "no", visibility);
		}


		void HiddenFloor::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("isSolidWhenHidden", isSolidWhenHidden);
			archive.Field("stayTime", stayTime);
			archive.Field("fadeTime", fadeTime);
			archive.Field("appearTime", appearTime);
			archive.Field("revealDistance", revealDistance);
		}
	}
}
