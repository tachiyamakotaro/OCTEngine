/**
 * @file Magnifier.cpp
 * @brief 虫眼鏡
 */
#include "stdafx.h"
#include "Magnifier.h"
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
			const ImU32 EDITOR_COLOR_LENS = IM_COL32(170, 230, 255, 255);
			const ImU32 EDITOR_COLOR_HANDLE = IM_COL32(150, 100, 60, 255);
			const ImU32 EDITOR_COLOR_GUIDE = IM_COL32(255, 230, 120, 70);
			const ImU32 EDITOR_COLOR_LIT = IM_COL32(255, 220, 90, 230);
			const ImU32 EDITOR_COLOR_HOT = IM_COL32(255, 120, 40, 255);

			/** 持ち手の長さ（直径に対する割合。モデルと合わせる） */
			constexpr float HANDLE_LENGTH_RATE = 0.6f;
			/** 光の線分の終わりが、レンズの上にあるとみなす距離 */
			constexpr float ON_LENS_DISTANCE = 0.5f;


			/** 点と線分の距離 */
			float DistanceToSegment(const math::Vec2& p, const math::Vec2& a, const math::Vec2& b)
			{
				const math::Vec2 ab = b - a;
				const float lengthSq = ab.LengthSq();
				const float t = lengthSq > 0.0f ? math::Clamp01(math::Dot(p - a, ab) / lengthSq) : 0.0f;
				return (p - (a + ab * t)).Length();
			}
		}


		Magnifier::Magnifier(const char* name, const float x, const float y, const float initialAngleDeg)
			: GimmickBase(name)
			, center(x, y)
			, angleDeg(initialAngleDeg)
		{
			focusDirection = GetAxis();
			coneLength = focalLength;
		}


		void Magnifier::Update(const GimmickUpdateContext& context)
		{
			// 光が当たっていれば、光が進んでいた向きに集まる。当たっていなければレンズの軸の向き（目印用）
			isLit = context.light.IsLit(*this);
			math::Vec2 incoming;
			if (isLit && FindIncomingDirection(context, incoming))
			{
				focusDirection = incoming;
			}
			else
			{
				focusDirection = GetAxis();
			}
			TraceFocus(context);
		}


		void Magnifier::ResetRuntimeState()
		{
			isLit = false;
			focusDirection = GetAxis();
			coneLength = focalLength;
			isSharp = true;
		}


		bool Magnifier::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			// レンズの線分に当たった光は止まり、点灯したことになる（集めた光は Update で自分で調べる）
			if (!RayIntersectSegment(origin, direction, GetLensStart(), GetLensEnd(), outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::Absorb;
			return true;
		}


		bool Magnifier::FindIncomingDirection(const GimmickUpdateContext& context, math::Vec2& outDirection) const
		{
			// 光の線分のうち、終わりの点がレンズの上にある物が「レンズに当たった光」
			const math::Vec2 lensStart = GetLensStart();
			const math::Vec2 lensEnd = GetLensEnd();
			for (const LightSegment& segment : context.light.segments)
			{
				if (DistanceToSegment(segment.end, lensStart, lensEnd) > ON_LENS_DISTANCE)
				{
					continue;
				}
				const math::Vec2 delta = segment.end - segment.start;
				if (delta.LengthSq() < 0.0001f)
				{
					continue;
				}
				outDirection = math::Normalize(delta);
				return true;
			}
			return false;
		}


		void Magnifier::TraceFocus(const GimmickUpdateContext& context)
		{
			// レンズの中心から焦点の少し先まで、光を止める物（ブロック・閉じた扉・燃える前の草など）を探す
			// 光を通り抜けさせる物（光の足場・ロープ）は止めない
			const float reach = focalLength + math::Max(focusTolerance, 0.0f);
			float nearest = reach;
			bool isBlocked = false;
			for (const std::unique_ptr<GimmickBase>& gimmick : context.gimmicks.GetAll())
			{
				if (gimmick.get() == this)
				{
					continue;
				}
				LightHit hit;
				if (gimmick->IntersectLight(center, focusDirection, hit)
					&& hit.response != LightResponse::PassThrough
					&& hit.distance < nearest)
				{
					nearest = hit.distance;
					isBlocked = true;
				}
			}

			// 何にも止められなければ、焦点で一点に集まる（空中、またはロープのような細い物の中）
			// 止められたら、その物の面まで。面が焦点の近くなら、そこで燃やせる
			coneLength = isBlocked ? nearest : focalLength;
			isSharp = math::Abs(coneLength - focalLength) <= math::Max(focusTolerance, 0.0f);
		}


		bool Magnifier::IsHotSpotIn(const math::Rect& rect, const float margin) const
		{
			if (!CanBurn())
			{
				return false;
			}
			const math::Vec2 point = GetFocusPoint();
			return point.x >= rect.min.x - margin && point.x <= rect.max.x + margin
				&& point.y >= rect.min.y - margin && point.y <= rect.max.y + margin;
		}


		math::Rect Magnifier::GetBounds() const
		{
			const float half = diameter * 0.5f;
			return { { center.x - half, center.y - half }, { center.x + half, center.y + half } };
		}


		bool Magnifier::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			// レンズの円の中か、レンズの線分の近く
			return (point - center).Length() <= diameter * 0.5f
				|| DistanceToSegment(point, GetLensStart(), GetLensEnd()) <= pickDistance;
		}


		void Magnifier::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			const math::Vec2 lensStart = GetLensStart();
			const math::Vec2 lensEnd = GetLensEnd();
			const math::Vec2 focus = GetFocusPoint();

			// 集まる光（レンズの両端から焦点へ）。光が当たっている間は明るく、燃やせるときは焦点を赤く
			if (isLit || isShowGuide)
			{
				const ImU32 coneColor = isLit ? EDITOR_COLOR_LIT : EDITOR_COLOR_GUIDE;
				canvas.Line(lensStart, focus, coneColor, 1.0f);
				canvas.Line(lensEnd, focus, coneColor, 1.0f);
				canvas.Dot(focus, CanBurn() ? 5.0f : 3.0f, CanBurn() ? EDITOR_COLOR_HOT : coneColor);
			}
			// 焦点の位置（さえぎられていても、本当の焦点に小さな印）
			canvas.Dot(center + focusDirection * focalLength, 2.0f, EDITOR_COLOR_GUIDE);

			// 持ち手とレンズ
			canvas.Line(lensStart, lensStart - GetLensDirection() * (diameter * HANDLE_LENGTH_RATE), EDITOR_COLOR_HANDLE, 3.0f);
			canvas.Line(lensStart, lensEnd, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LENS), 4.0f);
			canvas.Label(lensEnd, name);
		}


		void Magnifier::DrawInspector(GimmickInspector& inspector)
		{
			ImGui::DragFloat2("Center", &center.x, 1.0f, 0.0f, 0.0f, "%.1f");
			inspector.EditAngle("Axis", angleDeg, 45.0f);
			ImGui::DragFloat("Diameter", &diameter, 1.0f, 10.0f, 1000.0f, "%.1f");
			ImGui::DragFloat("Focal Length", &focalLength, 1.0f, 10.0f, 3000.0f, "%.1f");
			ImGui::DragFloat("Focus Tolerance", &focusTolerance, 0.5f, 0.0f, 500.0f, "%.1f");
			ImGui::Checkbox("Show Guide", &isShowGuide);
			ImGui::Text("Lit: %s / Sharp: %s / Cone: %.0f", isLit ? "YES" : "no", isSharp ? "YES" : "no", coneLength);
		}


		void Magnifier::Serialize(GimmickArchive& archive)
		{
			archive.Field("center", center);
			archive.Field("angleDeg", angleDeg);
			archive.Field("diameter", diameter);
			archive.Field("focalLength", focalLength);
			archive.Field("focusTolerance", focusTolerance);
			archive.Field("isShowGuide", isShowGuide);
			if (archive.IsReading())
			{
				ResetRuntimeState();
			}
		}
	}
}
