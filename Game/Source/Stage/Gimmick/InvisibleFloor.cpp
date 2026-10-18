/**
 * @file InvisibleFloor.cpp
 * @brief 透明の足場
 */
#include "stdafx.h"
#include "InvisibleFloor.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "GimmickManager.h"
#include "MistEmitter.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_LINE = IM_COL32(200, 225, 245, 255);
			const ImU32 EDITOR_COLOR_FILL = IM_COL32(200, 225, 245, 150);
		}


		InvisibleFloor::InvisibleFloor(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void InvisibleFloor::Update(const GimmickUpdateContext& context)
		{
			isRevealedThisFrame = false;
			if (isRevealed)
			{
				// 露が残るので、一度見えたら見えたまま
				revealedTime += context.deltaTime;
				return;
			}
			for (const MistEmitter& mist : context.gimmicks.View<MistEmitter>())
			{
				// 霧が出ていて、霧の範囲の中で、霧の先端が足場の上面まで下りてきたら見える
				if (mist.IsActive() && rect.Overlaps(mist.rect) && mist.GetFrontY() <= rect.max.y)
				{
					isRevealed = true;
					isRevealedThisFrame = true;
					revealedTime = 0.0f;
					return;
				}
			}
		}


		void InvisibleFloor::ResetRuntimeState()
		{
			isRevealed = false;
			isRevealedThisFrame = false;
			revealedTime = 0.0f;
		}


		CollisionKind InvisibleFloor::GetCollision(math::Rect& outRect) const
		{
			if (!IsSolid())
			{
				return CollisionKind::None;
			}
			outRect = rect;
			return CollisionKind::Terrain;
		}


		void InvisibleFloor::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 見えるようになったら塗る
			canvas.Rect(rect, isRevealed ? EDITOR_COLOR_FILL : 0, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE));
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void InvisibleFloor::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::Checkbox("Solid when hidden", &isSolidWhenHidden);
			ImGui::SetItemTooltip("ON: you can stand on it before the mist reveals it.");
			ImGui::Checkbox("Revealed", &isRevealed);
			ImGui::Text("Solid: %s", IsSolid() ? "YES" : "no");
		}


		void InvisibleFloor::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("isSolidWhenHidden", isSolidWhenHidden);
		}
	}
}
