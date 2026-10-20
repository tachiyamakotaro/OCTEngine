/**
 * @file SunLight.cpp
 * @brief 日光
 */
#include "stdafx.h"
#include "SunLight.h"
#include <cmath>
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "GimmickManager.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_WINDOW = IM_COL32(255, 220, 110, 255);
			const ImU32 EDITOR_COLOR_RAY = IM_COL32(255, 230, 140, 140);
			const ImU32 EDITOR_COLOR_OFF = IM_COL32(90, 90, 100, 255);

			/** 光の本数の上限（間隔を小さくしすぎたときに重くならないように） */
			constexpr int MAX_RAYS = 128;
			/** 間隔の下限 */
			constexpr float MIN_RAY_SPACING = 1.0f;
			/** エディタで光の向きを描く矢印の長さ（ピクセル） */
			constexpr float EDITOR_ARROW_PX = 30.0f;
			/** エディタで矢印を描く本数の目安 */
			constexpr int EDITOR_ARROW_COUNT = 5;


			/** 点と線分の距離 */
			float DistanceToSegment(const math::Vec2& p, const math::Vec2& a, const math::Vec2& b)
			{
				const math::Vec2 ab = b - a;
				const float lengthSq = ab.LengthSq();
				const float t = lengthSq > 0.0f ? math::Clamp01(math::Dot(p - a, ab) / lengthSq) : 0.0f;
				return (p - (a + ab * t)).Length();
			}
		}


		SunLight::SunLight(const char* name, const float x, const float y, const float initialWidth, const float initialAngleDeg)
			: GimmickBase(name)
			, center(x, y)
			, width(initialWidth)
			, angleDeg(initialAngleDeg)
		{}


		void SunLight::Update(const GimmickUpdateContext& context)
		{
			// 信号が空なら使わない（IsShining で isOn だけを見る）
			isSignalOn = context.gimmicks.IsSignalOn(signal);
		}


		int SunLight::GetRayCount() const
		{
			const float spacing = math::Max(raySpacing, MIN_RAY_SPACING);
			const int count = static_cast<int>(std::floor(math::Max(width, 0.0f) / spacing + 0.5f));
			return count < 1 ? 1 : (count > MAX_RAYS ? MAX_RAYS : count);
		}


		void SunLight::CollectEmittedRays(std::vector<EmittedRay>& out) const
		{
			if (!IsShining())
			{
				return;
			}
			// 窓を本数で等分し、それぞれの真ん中から光を出す
			const int count = GetRayCount();
			const math::Vec2 start = GetStart();
			const math::Vec2 step = GetWindowDirection() * (width / static_cast<float>(count));
			const math::Vec2 direction = GetDirection();
			for (int i = 0; i < count; ++i)
			{
				EmittedRay ray;
				ray.origin = start + step * (static_cast<float>(i) + 0.5f);
				ray.direction = direction;
				ray.width = math::Max(rayWidth, 0.0f);
				out.push_back(ray);
			}
		}


		void SunLight::RenameSignal(const std::string& oldName, const std::string& newName)
		{
			if (signal == oldName) { signal = newName; }
		}


		void SunLight::RemoveSignal(const std::string& signalName)
		{
			if (signal == signalName) { signal.clear(); }
		}


		math::Rect SunLight::GetBounds() const
		{
			const math::Vec2 a = GetStart();
			const math::Vec2 b = GetEnd();
			return { { math::Min(a.x, b.x), math::Min(a.y, b.y) }, { math::Max(a.x, b.x), math::Max(a.y, b.y) } };
		}


		bool SunLight::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			return DistanceToSegment(point, GetStart(), GetEnd()) <= pickDistance;
		}


		void SunLight::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 光の向きの矢印を何本か（窓の端から端まで）
			const math::Vec2 start = GetStart();
			const math::Vec2 end = GetEnd();
			const math::Vec2 arrow = GetDirection() * canvas.PixelsToWorld(EDITOR_ARROW_PX);
			for (int i = 0; i < EDITOR_ARROW_COUNT; ++i)
			{
				const float t = (static_cast<float>(i) + 0.5f) / static_cast<float>(EDITOR_ARROW_COUNT);
				const math::Vec2 from = start + (end - start) * t;
				canvas.Line(from, from + arrow, IsShining() ? EDITOR_COLOR_RAY : EDITOR_COLOR_OFF, 1.0f);
			}
			// 窓
			canvas.Line(start, end, EditorCanvas::Pick(isSelected, IsShining() ? EDITOR_COLOR_WINDOW : EDITOR_COLOR_OFF), 4.0f);
			canvas.Label(end, name + (signal.empty() ? "" : " <- " + signal));
		}


		void SunLight::DrawInspector(GimmickInspector& inspector)
		{
			ImGui::DragFloat2("Center", &center.x, 1.0f, 0.0f, 0.0f, "%.1f");
			ImGui::DragFloat("Width", &width, 1.0f, 1.0f, 5000.0f, "%.0f");
			inspector.EditAngle("Window Angle", windowAngleDeg, 90.0f);
			ImGui::SetItemTooltip("0 = a skylight (horizontal). 90 = a window in a wall.");
			inspector.EditAngle("Light Angle", angleDeg, 15.0f);
			ImGui::SetItemTooltip("Direction of the sunlight. -90 = straight down, -60 = down and to the right.");
			ImGui::DragFloat("Ray Spacing", &raySpacing, 0.5f, MIN_RAY_SPACING, 500.0f, "%.1f");
			ImGui::SetItemTooltip("Smaller = finer shadows, but more rays.");
			ImGui::DragFloat("Ray Width", &rayWidth, 0.5f, 0.0f, 500.0f, "%.1f");
			ImGui::SetItemTooltip("Only for drawing. Same as Ray Spacing = no gaps between rays.");
			ImGui::Text("Rays: %d", GetRayCount());
			ImGui::Checkbox("On", &isOn);
			inspector.SignalCombo("Signal", signal);
			ImGui::SetItemTooltip("(none) = always shining. Otherwise it shines only while the signal is ON.");
			ImGui::Text("Shining: %s", IsShining() ? "YES" : "no");
		}


		void SunLight::Serialize(GimmickArchive& archive)
		{
			archive.Field("center", center);
			archive.Field("width", width);
			archive.Field("windowAngleDeg", windowAngleDeg);
			archive.Field("angleDeg", angleDeg);
			archive.Field("raySpacing", raySpacing);
			archive.Field("rayWidth", rayWidth);
			archive.Field("isOn", isOn);
			archive.Field("signal", signal);
		}
	}
}
