/**
 * @file BurnableGimmick.cpp
 * @brief 燃える物の共通部分
 */
#include "stdafx.h"
#include "BurnableGimmick.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "GimmickManager.h"
#include "Magnifier.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_BURNING = IM_COL32(255, 120, 30, 255);
			const ImU32 EDITOR_COLOR_BURNT = IM_COL32(90, 80, 70, 160);
			const ImU32 EDITOR_COLOR_HEAT = IM_COL32(255, 80, 40, 255);
			const ImU32 EDITOR_COLOR_MARK = IM_COL32(15, 15, 15, 255);
			const ImU32 EDITOR_COLOR_MARK_RIM = IM_COL32(255, 255, 255, 200);

			/** 燃え広がるとき「触れている」とみなす距離（ぴったり接している物どうしも燃え移るように少し広げる） */
			constexpr float SPREAD_MARGIN = 3.0f;


			/** 矩形を margin だけ広げる */
			math::Rect Expand(const math::Rect& rect, const float margin)
			{
				return { { rect.min.x - margin, rect.min.y - margin }, { rect.max.x + margin, rect.max.y + margin } };
			}


			/** 点を矩形の中に収める */
			math::Vec2 ClampInto(const math::Vec2& point, const math::Rect& rect)
			{
				return { math::Clamp(point.x, rect.min.x, rect.max.x), math::Clamp(point.y, rect.min.y, rect.max.y) };
			}


			/** 2つの色を t（0〜1）で混ぜる */
			ImU32 LerpColor(const ImU32 a, const ImU32 b, const float t)
			{
				const ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
				const ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
				return ImGui::ColorConvertFloat4ToU32(ImVec4(
					math::Lerp(ca.x, cb.x, t), math::Lerp(ca.y, cb.y, t), math::Lerp(ca.z, cb.z, t), math::Lerp(ca.w, cb.w, t)));
			}
		}


		BurnableGimmick::BurnableGimmick(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{
			heatPoint = (rect.min + rect.max) * 0.5f;
		}


		void BurnableGimmick::Update(const GimmickUpdateContext& context)
		{
			isIgnitedThisFrame = false;
			isBurntThisFrame = false;
			isHeated = false;
			hasMark = false;
			if (isBurnt)
			{
				return;
			}

			// 燃えている：時間がたったら燃え尽きる
			if (isBurning)
			{
				burnElapsed += context.deltaTime;
				if (burnElapsed >= burnTime)
				{
					isBurning = false;
					isBurnt = true;
					isBurntThisFrame = true;
				}
				return;
			}

			const math::Rect current = GetCurrentRect();

			// 0. 黒い印：虫眼鏡の軸の向きの焦点（光を当てたら燃える所）が自分の上にあれば、そこに付ける
			if (isMarked)
			{
				for (const Magnifier& magnifier : context.gimmicks.View<Magnifier>())
				{
					if (magnifier.IsGuidePointIn(current, GetHotSpotMargin()))
					{
						hasMark = true;
						markPoint = ClampInto(magnifier.guidePoint, current);
						break;
					}
				}
			}

			// 1. 虫眼鏡の焦点が当たっていれば温まり、外れていれば冷める
			for (const Magnifier& magnifier : context.gimmicks.View<Magnifier>())
			{
				if (magnifier.IsHotSpotIn(current, GetHotSpotMargin()))
				{
					isHeated = true;
					heatPoint = ClampInto(magnifier.GetFocusPoint(), current);
					break;
				}
			}
			if (isHeated)
			{
				heat += context.deltaTime / math::Max(igniteTime, 0.01f);
			}
			else
			{
				heat = math::MoveTowards(heat, 0.0f, context.deltaTime / math::Max(coolTime, 0.01f));
			}
			heat = math::Clamp01(heat);

			// 2. 触れている物が燃えていて、spreadTime 秒たっていたら燃え移る
			//    燃えている物は縮んでいくので、触れているかは「元の範囲」どうしで調べる
			bool isSpread = false;
			const math::Rect touchArea = Expand(rect, SPREAD_MARGIN);
			for (const BurnableGimmick& other : context.gimmicks.View<BurnableGimmick>())
			{
				if (&other == this || !other.isBurning || other.spreadTime < 0.0f || other.burnElapsed < other.spreadTime)
				{
					continue;
				}
				if (touchArea.Overlaps(other.rect))
				{
					isSpread = true;
					heatPoint = ClampInto((other.rect.min + other.rect.max) * 0.5f, current);
					break;
				}
			}

			if (heat >= 1.0f || isSpread)
			{
				Ignite();
			}
		}


		void BurnableGimmick::ResetRuntimeState()
		{
			heat = 0.0f;
			isHeated = false;
			isBurning = false;
			burnElapsed = 0.0f;
			isBurnt = false;
			isIgnitedThisFrame = false;
			isBurntThisFrame = false;
			heatPoint = (rect.min + rect.max) * 0.5f;
			hasMark = false;
		}


		void BurnableGimmick::Ignite()
		{
			if (isBurning || isBurnt)
			{
				return;
			}
			isBurning = true;
			burnElapsed = 0.0f;
			heat = 1.0f;
			isIgnitedThisFrame = true;
		}


		void BurnableGimmick::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			if (isBurnt)
			{
				// 燃え尽きた：元の範囲を薄い枠だけ
				canvas.Rect(rect, 0, EditorCanvas::Pick(isSelected, EDITOR_COLOR_BURNT));
			}
			else
			{
				// 温まるほど赤く、燃えている間はオレンジ。燃えて縮んだら元の範囲を枠で
				const ImU32 fill = isBurning ? EDITOR_COLOR_BURNING : LerpColor(GetEditorColor(), EDITOR_COLOR_HEAT, heat);
				canvas.Rect(GetCurrentRect(), fill, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
				if (isBurning)
				{
					canvas.Rect(rect, 0, EDITOR_COLOR_BURNING);
				}
				if (hasMark && !isBurning)
				{
					// 黒い印（白い縁取りで、見取り図の暗い背景でも見えるように）
					canvas.Dot(markPoint, 6.0f, EDITOR_COLOR_MARK_RIM);
					canvas.Dot(markPoint, 4.5f, EDITOR_COLOR_MARK);
				}
				if (isHeated)
				{
					canvas.Dot(heatPoint, 4.0f, EDITOR_COLOR_HEAT);
				}
			}
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void BurnableGimmick::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::DragFloat("Ignite Time", &igniteTime, 0.05f, 0.05f, 30.0f, "%.2f s");
			ImGui::DragFloat("Cool Time", &coolTime, 0.05f, 0.05f, 30.0f, "%.2f s");
			ImGui::DragFloat("Burn Time", &burnTime, 0.05f, 0.05f, 30.0f, "%.2f s");
			ImGui::DragFloat("Spread Time", &spreadTime, 0.05f, -1.0f, 30.0f, "%.2f s");
			ImGui::SetItemTooltip("Seconds after catching fire until the fire spreads to touching burnables (negative = never)");
			ImGui::Checkbox("Black Mark", &isMarked);
			ImGui::SetItemTooltip("Blacken the spot where a magnifier focuses (shows where to burn)");
			ImGui::Text("Heat: %.0f%%  %s", heat * 100.0f, isBurnt ? "BURNT" : (isBurning ? "BURNING" : (isHeated ? "heating" : "")));
			if (ImGui::Button("Ignite"))
			{
				Ignite();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset Fire"))
			{
				ResetRuntimeState();
			}
		}


		void BurnableGimmick::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("igniteTime", igniteTime);
			archive.Field("coolTime", coolTime);
			archive.Field("burnTime", burnTime);
			archive.Field("spreadTime", spreadTime);
			archive.Field("isMarked", isMarked);
			if (archive.IsReading())
			{
				ResetRuntimeState();
			}
		}
	}
}
