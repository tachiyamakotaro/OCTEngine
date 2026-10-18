/**
 * @file LightMirror.cpp
 * @brief 光を反射する鏡（線分）
 */
#include "stdafx.h"
#include "LightMirror.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_MIRROR = IM_COL32(230, 240, 255, 255);
			const ImU32 EDITOR_COLOR_MIRROR_BACK = IM_COL32(90, 90, 100, 255);
			const ImU32 EDITOR_COLOR_RAIL = IM_COL32(230, 240, 255, 90);


			/** 点と線分の距離 */
			float DistanceToSegment(const math::Vec2& p, const math::Vec2& a, const math::Vec2& b)
			{
				const math::Vec2 ab = b - a;
				const float lengthSq = ab.LengthSq();
				const float t = lengthSq > 0.0f ? math::Clamp01(math::Dot(p - a, ab) / lengthSq) : 0.0f;
				return (p - (a + ab * t)).Length();
			}
		}


		LightMirror::LightMirror(const char* name, const float x, const float y, const float initialAngleDeg, const float initialLength)
			: GimmickBase(name)
			, center(x, y)
			, angleDeg(initialAngleDeg)
			, length(initialLength)
		{}


		bool LightMirror::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (!RayIntersectSegment(origin, direction, GetStart(), GetEnd(), outHit.distance))
			{
				return false;
			}
			const math::Vec2 normal = GetNormal();
			const bool isFront = math::Dot(direction, normal) < 0.0f;
			if (isFront)
			{
				outHit.response = LightResponse::Reflect;
				outHit.normal = normal;
			}
			else if (isDoubleSided)
			{
				// 裏側も鏡なので、裏向きの法線で反射する
				outHit.response = LightResponse::Reflect;
				outHit.normal = normal * -1.0f;
			}
			else
			{
				// 裏側に当たった
				outHit.response = LightResponse::Stop;
			}
			return true;
		}


		void LightMirror::SetAnchor(const math::Vec2& anchor)
		{
			const float dx = anchor.x - center.x;
			center = anchor;
			railMinX += dx;
			railMaxX += dx;
		}


		math::Rect LightMirror::GetBounds() const
		{
			const math::Vec2 a = GetStart();
			const math::Vec2 b = GetEnd();
			return { { math::Min(a.x, b.x), math::Min(a.y, b.y) }, { math::Max(a.x, b.x), math::Max(a.y, b.y) } };
		}


		bool LightMirror::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			return DistanceToSegment(point, GetStart(), GetEnd()) <= pickDistance;
		}


		void LightMirror::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// レール
			if (hasRail)
			{
				canvas.Line({ railMinX, center.y }, { railMaxX, center.y }, EDITOR_COLOR_RAIL, 1.0f);
			}
			// 表を明るく、裏を暗く。中心から表側に短い線
			const ImU32 color = EditorCanvas::Pick(isSelected, EDITOR_COLOR_MIRROR);
			const math::Vec2 backOffset = GetNormal() * -canvas.PixelsToWorld(2.0f);
			canvas.Line(GetStart() + backOffset, GetEnd() + backOffset, isDoubleSided ? EDITOR_COLOR_MIRROR : EDITOR_COLOR_MIRROR_BACK, 3.0f);
			canvas.Line(GetStart(), GetEnd(), color, 3.0f);
			canvas.Line(center, center + GetNormal() * canvas.PixelsToWorld(12.0f), color, 1.0f);
			canvas.Label(GetEnd(), name);
		}


		void LightMirror::DrawInspector(GimmickInspector& inspector)
		{
			ImGui::DragFloat2("Center", &center.x, 1.0f, 0.0f, 0.0f, "%.1f");
			inspector.EditAngle("Angle", angleDeg, rotationStepDeg);
			ImGui::DragFloat("Length", &length, 1.0f, 5.0f, 2000.0f, "%.1f");
			ImGui::Checkbox("Double Sided", &isDoubleSided);
			ImGui::Separator();
			ImGui::Checkbox("Rotatable", &isRotatable);
			ImGui::DragFloat("Rotation Step", &rotationStepDeg, 1.0f, 1.0f, 180.0f, "%.0f deg");
			ImGui::Checkbox("Movable", &isMovable);
			ImGui::Checkbox("Rail", &hasRail);
			if (hasRail)
			{
				ImGui::DragFloat("Rail Min X", &railMinX, 1.0f, 0.0f, 0.0f, "%.1f");
				ImGui::DragFloat("Rail Max X", &railMaxX, 1.0f, 0.0f, 0.0f, "%.1f");
				if (railMaxX < railMinX) { railMaxX = railMinX; }
			}
		}


		void LightMirror::Serialize(GimmickArchive& archive)
		{
			archive.Field("center", center);
			archive.Field("angleDeg", angleDeg);
			archive.Field("length", length);
			archive.Field("isDoubleSided", isDoubleSided);
			archive.Field("isRotatable", isRotatable);
			archive.Field("rotationStepDeg", rotationStepDeg);
			archive.Field("isMovable", isMovable);
			archive.Field("hasRail", hasRail);
			archive.Field("railMinX", railMinX);
			archive.Field("railMaxX", railMaxX);
		}
	}
}
