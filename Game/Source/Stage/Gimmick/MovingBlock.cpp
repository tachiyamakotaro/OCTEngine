/**
 * @file MovingBlock.cpp
 * @brief ソーラーパネル付きの動く床・動く壁
 */
#include "stdafx.h"
#include "MovingBlock.h"
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
			/** ブロック（止まっているとき） */
			const ImU32 EDITOR_COLOR_BLOCK = IM_COL32(70, 120, 200, 255);
			/** ブロック（動いているとき） */
			const ImU32 EDITOR_COLOR_RUNNING = IM_COL32(110, 170, 245, 255);
			/** 道のり・始点・終点の枠 */
			const ImU32 EDITOR_COLOR_PATH = IM_COL32(110, 170, 245, 150);
			/** パネルの縁に光が当たったかを調べるときの許容差 */
			constexpr float PANEL_HIT_EPSILON = 0.01f;

			/** パネルの面の名前（プロパティ欄） */
			const char* const PANEL_SIDE_NAMES[] = { "Top", "Bottom", "Left", "Right" };


			/** 色を混ぜる（t=0 で a、t=1 で b） */
			ImU32 LerpColor(const ImU32 a, const ImU32 b, const float t)
			{
				const ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
				const ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
				return ImGui::ColorConvertFloat4ToU32(ImVec4(
					math::Lerp(ca.x, cb.x, t), math::Lerp(ca.y, cb.y, t), math::Lerp(ca.z, cb.z, t), math::Lerp(ca.w, cb.w, t)));
			}
		}


		MovingBlock::MovingBlock(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
			, previousRect(rect)
		{}


		void MovingBlock::Update(const GimmickUpdateContext& context)
		{
			// 動く前の範囲を覚えておく（乗っているキャラクターを、動いた分だけ運ぶため）
			previousRect = GetCurrentRect();

			// パネルに光が当たっているか（IntersectLight でパネルに当たったときだけ「点灯」あつかいになる）
			isLit = context.light.IsLit(*this);
			UpdateCharge(context.deltaTime);

			// 動いている間は終点へ、止まったら始点へ戻る（1フレームぶん動かしてみる）
			const MovePath before = movePath;
			movePath.MoveToward(isRunning ? 1.0f : 0.0f, context.deltaTime);
			const math::Vec2 delta = movePath.GetOffset() - before.GetOffset();

			// 体を地形との間にはさんでしまうなら、このフレームは動かない（動く前に戻す）
			if (IsBlockedByBody(context, GetCurrentRect(), delta))
			{
				movePath = before;
			}
		}


		void MovingBlock::UpdateCharge(const float deltaTime)
		{
			// 光が当たっている間、電気がたまる（chargeTime 秒で満タン）
			if (isLit)
			{
				charge = math::Min(charge + deltaTime / math::Max(chargeTime, 0.01f), 1.0f);
			}
			// 満タンになったら動き出す
			if (!isRunning && charge >= 1.0f)
			{
				isRunning = true;
			}
			// 動いている間、電気を使う（runTime 秒で空）。空になったら止まる（始点へ戻る）
			if (isRunning)
			{
				charge -= deltaTime / math::Max(runTime, 0.01f);
				if (charge <= 0.0f)
				{
					charge = 0.0f;
					isRunning = false;
				}
			}
		}


		void MovingBlock::ResetRuntimeState()
		{
			movePath.ResetRuntimeState();
			previousRect = rect;
			charge = 0.0f;
			isRunning = false;
			isLit = false;
		}


		CollisionKind MovingBlock::GetCollision(math::Rect& outRect) const
		{
			outRect = GetCurrentRect();
			return CollisionKind::Terrain;
		}


		bool MovingBlock::GetCarryDelta(const math::Rect& groundProbe, math::Vec2& outDelta) const
		{
			// 足元のすぐ下の箱が「動く前の範囲」に重なっていたら、この床の上に立っていた
			if (!groundProbe.Overlaps(previousRect))
			{
				return false;
			}
			outDelta = GetCurrentRect().min - previousRect.min;
			return true;
		}


		bool MovingBlock::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (!RayIntersectRect(origin, direction, GetCurrentRect(), outHit.distance))
			{
				return false;
			}
			// 当たった点がパネルの上なら吸収（電気になる）、それ以外の面なら止まるだけ
			const math::Vec2 point = origin + direction * outHit.distance;
			const math::Rect panel = GetPanelRect();
			const bool isOnPanel = point.x >= panel.min.x - PANEL_HIT_EPSILON && point.x <= panel.max.x + PANEL_HIT_EPSILON
				&& point.y >= panel.min.y - PANEL_HIT_EPSILON && point.y <= panel.max.y + PANEL_HIT_EPSILON;
			outHit.response = isOnPanel ? LightResponse::Absorb : LightResponse::Stop;
			return true;
		}


		math::Rect MovingBlock::CalcPanelRect(const math::Rect& block) const
		{
			// 横の面（上・下）なら幅、縦の面（左・右）なら高さに沿って並べる
			const bool isHorizontalFace = panelSide == PanelSide::Top || panelSide == PanelSide::Bottom;
			const float faceLength = isHorizontalFace ? block.Width() : block.Height();
			const float length = (panelLength <= 0.0f) ? faceLength : math::Min(panelLength, faceLength);
			const float thickness = math::Min(PANEL_THICKNESS, isHorizontalFace ? block.Height() : block.Width());
			const math::Vec2 center = (block.min + block.max) * 0.5f;

			switch (panelSide)
			{
			case PanelSide::Top:
				return math::Rect::FromMinSize(center.x - length * 0.5f, block.max.y - thickness, length, thickness);
			case PanelSide::Bottom:
				return math::Rect::FromMinSize(center.x - length * 0.5f, block.min.y, length, thickness);
			case PanelSide::Left:
				return math::Rect::FromMinSize(block.min.x, center.y - length * 0.5f, thickness, length);
			case PanelSide::Right:
			default:
				return math::Rect::FromMinSize(block.max.x - thickness, center.y - length * 0.5f, thickness, length);
			}
		}


		bool MovingBlock::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			// 置いた位置と、今の位置のどちらをクリックしても選べるようにする
			auto isInside = [&point](const math::Rect& r)
				{
					return point.x >= r.min.x && point.x <= r.max.x && point.y >= r.min.y && point.y <= r.max.y;
				};
			return isInside(rect) || isInside(GetCurrentRect());
		}


		void MovingBlock::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 道のり（始点の中心 → 終点の中心）と、終点の枠
			const math::Vec2 startCenter = (rect.min + rect.max) * 0.5f;
			movePath.DrawEditor(canvas, startCenter, EDITOR_COLOR_PATH);
			canvas.Rect(GetEndRect(), 0, EDITOR_COLOR_PATH);
			// 置いた位置（始点）の枠
			canvas.Rect(rect, 0, EditorCanvas::Pick(isSelected, EDITOR_COLOR_PATH));
			// 今の位置を塗りで
			canvas.Rect(GetCurrentRect(), isRunning ? EDITOR_COLOR_RUNNING : EDITOR_COLOR_BLOCK, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
			// パネル（ためている間は黒 → 灰色 → 白、動いている間は白のまま）
			canvas.Rect(GetPanelRect(), LerpColor(IM_COL32(15, 15, 18, 255), IM_COL32(245, 248, 255, 255), GetPanelBrightness()), 0);
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void MovingBlock::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			movePath.DrawInspector();

			ImGui::SeparatorText("Solar Panel");
			int sideIndex = static_cast<int>(panelSide);
			if (ImGui::Combo("Panel Side", &sideIndex, PANEL_SIDE_NAMES, static_cast<int>(PanelSide::Num)))
			{
				panelSide = static_cast<PanelSide>(sideIndex);
			}
			ImGui::DragFloat("Panel Length", &panelLength, 1.0f, 0.0f, 10000.0f, "%.0f");
			ImGui::SetItemTooltip("0 = the whole face.");
			ImGui::DragFloat("Charge Time", &chargeTime, 0.05f, 0.05f, 30.0f, "%.2f s");
			ImGui::SetItemTooltip("Seconds of light to fill the panel.");
			ImGui::DragFloat("Run Time", &runTime, 0.05f, 0.05f, 60.0f, "%.2f s");
			ImGui::SetItemTooltip("Seconds it keeps running on a full charge.");
			ImGui::ProgressBar(charge, ImVec2(-1.0f, 0.0f));
			ImGui::Text("Lit: %s   Running: %s", isLit ? "YES" : "no", isRunning ? "YES" : "no");
		}


		void MovingBlock::Rotate90(const int quarterTurns)
		{
			// 形を回して、動く向きとパネルの面も一緒に回す（上に動く床 → 左に動く壁）
			RectGimmick::Rotate90(quarterTurns);
			movePath.Rotate90(quarterTurns);
			// 反時計回りに90度：上 → 左 → 下 → 右 → 上
			const int turns = ((quarterTurns % 4) + 4) % 4;
			for (int i = 0; i < turns; ++i)
			{
				switch (panelSide)
				{
				case PanelSide::Top:	panelSide = PanelSide::Left; break;
				case PanelSide::Left:	panelSide = PanelSide::Bottom; break;
				case PanelSide::Bottom:	panelSide = PanelSide::Right; break;
				default:				panelSide = PanelSide::Top; break;
				}
			}
		}


		void MovingBlock::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			movePath.Serialize(archive);
			// enum はそのまま保存できないので、数字にして保存する（0=上, 1=下, 2=左, 3=右）
			int sideValue = static_cast<int>(panelSide);
			archive.Field("panelSide", sideValue);
			if (archive.IsReading())
			{
				const bool isValid = sideValue >= 0 && sideValue < static_cast<int>(PanelSide::Num);
				panelSide = isValid ? static_cast<PanelSide>(sideValue) : PanelSide::Top;
			}
			archive.Field("panelLength", panelLength);
			archive.Field("chargeTime", chargeTime);
			archive.Field("runTime", runTime);
		}


		bool MovingBlock::IsBlockedByBody(const GimmickUpdateContext& context, const math::Rect& nextRect, const math::Vec2& delta) const
		{
			if (delta.x == 0.0f && delta.y == 0.0f)
			{
				return false;
			}
			for (const math::Rect& body : context.bodies)
			{
				// 動いた先が体に重ならなければ、押さない
				if (!nextRect.Overlaps(body))
				{
					continue;
				}
				// 体を動いた分だけ押した先が、ほかの地形でふさがっていたら、はさんでしまう
				if (context.gimmicks.OverlapsTerrain(Translate(body, delta)))
				{
					return true;
				}
			}
			return false;
		}
	}
}
