/**
 * @file FrontWall.cpp
 * @brief 手前の壁
 */
#include "stdafx.h"
#include "FrontWall.h"
#include "DarkZone.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "GimmickManager.h"
#include <cstdio>


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_FILL = IM_COL32(80, 100, 140, 40);
			const ImU32 EDITOR_COLOR_LINE = IM_COL32(150, 180, 230, 255);
			const ImU32 EDITOR_COLOR_LINK = IM_COL32(150, 180, 230, 120);
		}


		FrontWall::FrontWall(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		bool FrontWall::IsLinkedTo(const DarkZone& zone) const
		{
			if (!darkZone.empty())
			{
				return zone.name == darkZone;
			}
			// 名前を決めていないときは、壁と重なっている暗がり全部（ステージ全体の暗がりは除く）
			return !zone.isWholeStage && zone.GetDarkBounds().Overlaps(rect);
		}


		void FrontWall::Update(const GimmickUpdateContext& context)
		{
			// プレイヤーが暗い部屋に入ったか（体の真ん中が紐づいた暗がりの中、または体が壁の範囲に入った）
			isPlayerInside = false;
			for (const math::Rect& body : context.bodies)
			{
				if (body.Overlaps(rect))
				{
					isPlayerInside = true;
					break;
				}
				const math::Vec2 center = (body.min + body.max) * 0.5f;
				for (const DarkZone& zone : context.gimmicks.View<DarkZone>())
				{
					if (IsLinkedTo(zone) && zone.Contains(center))
					{
						isPlayerInside = true;
						break;
					}
				}
				if (isPlayerInside)
				{
					break;
				}
			}

			// 入っている間は透けていき、出ると戻ってくる（fadeTime 秒で 0 ⇔ 1）
			const float target = isPlayerInside ? 0.0f : 1.0f;
			const float step = fadeTime > 0.0f ? context.deltaTime / fadeTime : 1.0f;
			if (visibility < target)
			{
				visibility = math::Min(visibility + step, target);
			}
			else
			{
				visibility = math::Max(visibility - step, target);
			}
		}


		void FrontWall::ResetRuntimeState()
		{
			visibility = 1.0f;
			isPlayerInside = false;
		}


		bool FrontWall::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			// 枠の線の近く（内側と外側に pickDistance）だけ当たり
			const bool isInOuter = point.x >= rect.min.x - pickDistance && point.x <= rect.max.x + pickDistance
				&& point.y >= rect.min.y - pickDistance && point.y <= rect.max.y + pickDistance;
			const bool isInInner = point.x > rect.min.x + pickDistance && point.x < rect.max.x - pickDistance
				&& point.y > rect.min.y + pickDistance && point.y < rect.max.y - pickDistance;
			return isInOuter && !isInInner;
		}


		void FrontWall::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 中のギミックが見えるように、薄く塗って枠を描く
			canvas.Rect(rect, EDITOR_COLOR_FILL, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE));
			// 対角線を引いて「手前をふさいでいる」ことがわかるようにする
			canvas.Line(rect.min, rect.max, EDITOR_COLOR_LINK);
			canvas.Line({ rect.min.x, rect.max.y }, { rect.max.x, rect.min.y }, EDITOR_COLOR_LINK);
			canvas.Label({ rect.min.x, rect.max.y }, name + " (front wall" + (darkZone.empty() ? ")" : " -> " + darkZone + ")"));
		}


		void FrontWall::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), "%s", darkZone.c_str());
			if (ImGui::InputText("Dark Zone", buffer, sizeof(buffer)))
			{
				darkZone = buffer;
			}
			ImGui::SetItemTooltip("Name of the Dark Zone this wall belongs to.\nEmpty = every Dark Zone that overlaps this wall.");
			ImGui::DragFloat("Fade Time", &fadeTime, 0.01f, 0.0f, 5.0f, "%.2f s");
			ImGui::ColorEdit3("Color", color);
			ImGui::DragFloat("Stone Scale", &stoneScale, 1.0f, 10.0f, 2000.0f, "%.0f");
			ImGui::DragFloat("Edge Feather", &edgeFeather, 1.0f, 0.0f, 500.0f, "%.0f");
			ImGui::DragFloat("Rim Width", &rimWidth, 1.0f, 0.0f, 500.0f, "%.0f");
			ImGui::Text("Visibility: %.2f  %s", visibility, isPlayerInside ? "(player inside)" : "");
			ImGui::TextDisabled("Click the frame line to select this wall.");
		}


		void FrontWall::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("darkZone", darkZone);
			archive.Field("fadeTime", fadeTime);
			archive.Field("colorR", color[0]);
			archive.Field("colorG", color[1]);
			archive.Field("colorB", color[2]);
			archive.Field("stoneScale", stoneScale);
			archive.Field("edgeFeather", edgeFeather);
			archive.Field("rimWidth", rimWidth);
		}
	}
}
