/**
 * @file LightPlatform.cpp
 * @brief 光の足場
 */
#include "stdafx.h"
#include "LightPlatform.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_OFF = IM_COL32(90, 200, 230, 120);
			const ImU32 EDITOR_COLOR_ON = IM_COL32(90, 200, 230, 200);
		}


		LightPlatform::LightPlatform(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void LightPlatform::Update(const GimmickUpdateContext& context)
		{
			isLit = context.light.IsLit(*this);
			if (isLit)
			{
				// 光が当たっている間は満タン、外れたら猶予時間だけ残る
				solidTimer = lingerTime > context.deltaTime ? lingerTime : context.deltaTime;
			}
			else if (solidTimer > 0.0f)
			{
				solidTimer -= context.deltaTime;
			}
		}


		void LightPlatform::ResetRuntimeState()
		{
			isLit = false;
			solidTimer = 0.0f;
		}


		CollisionKind LightPlatform::GetCollision(math::Rect& outRect) const
		{
			if (!IsSolid())
			{
				return CollisionKind::None;
			}
			outRect = rect;
			return CollisionKind::Platform;
		}


		bool LightPlatform::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (!RayIntersectRect(origin, direction, rect, outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::PassThrough;
			return true;
		}


		void LightPlatform::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			canvas.Rect(rect, IsSolid() ? EDITOR_COLOR_ON : 0, EditorCanvas::Pick(isSelected, EDITOR_COLOR_OFF));
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void LightPlatform::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::DragFloat("Linger Time", &lingerTime, 0.01f, 0.0f, 5.0f, "%.2f s");
			ImGui::Text("Solid: %s", IsSolid() ? "YES" : "no");
		}


		void LightPlatform::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("lingerTime", lingerTime);
		}
	}
}
