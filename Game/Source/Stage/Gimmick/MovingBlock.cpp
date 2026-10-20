/**
 * @file MovingBlock.cpp
 * @brief 動く床・動く壁
 */
#include "stdafx.h"
#include "MovingBlock.h"
#include <algorithm>
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
			/** 止まっているとき */
			const ImU32 EDITOR_COLOR_OFF = IM_COL32(100, 125, 160, 255);
			/** 動いてよいとき */
			const ImU32 EDITOR_COLOR_ON = IM_COL32(120, 200, 235, 255);
			/** 道のり・始点・終点の枠 */
			const ImU32 EDITOR_COLOR_PATH = IM_COL32(120, 200, 235, 150);
		}


		MovingBlock::MovingBlock(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
			, previousRect(rect)
		{}


		void MovingBlock::Update(const GimmickUpdateContext& context)
		{
			// 動く前の範囲を覚えておく（乗っているキャラクターを、動いた分だけ運ぶため）
			previousRect = GetCurrentRect();

			// つながった信号が全部 ON なら動いてよい（つながりがなければ動かない）
			isPowered = !requiredSignals.empty();
			for (const std::string& signalName : requiredSignals)
			{
				if (!context.gimmicks.IsSignalOn(signalName))
				{
					isPowered = false;
					break;
				}
			}

			// 1フレームぶん動かしてみる
			const MovePath before = movePath;
			movePath.Update(isPowered, context.deltaTime);
			const math::Vec2 delta = movePath.GetOffset() - before.GetOffset();

			// 体を地形との間にはさんでしまうなら、このフレームは動かない（動く前に戻す）
			if (IsBlockedByBody(context, GetCurrentRect(), delta))
			{
				movePath = before;
			}
		}


		void MovingBlock::ResetRuntimeState()
		{
			movePath.ResetRuntimeState();
			previousRect = rect;
			isPowered = false;
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
			outHit.response = LightResponse::Stop;
			return true;
		}


		void MovingBlock::RenameSignal(const std::string& oldName, const std::string& newName)
		{
			for (std::string& required : requiredSignals)
			{
				if (required == oldName) { required = newName; }
			}
		}


		void MovingBlock::RemoveSignal(const std::string& signalName)
		{
			requiredSignals.erase(std::remove(requiredSignals.begin(), requiredSignals.end(), signalName), requiredSignals.end());
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
			canvas.Rect(GetCurrentRect(), isPowered ? EDITOR_COLOR_ON : EDITOR_COLOR_OFF, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void MovingBlock::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			movePath.DrawInspector();
			ImGui::Text("Powered: %s", isPowered ? "YES" : "no");
			ImGui::TextUnformatted("Required signals (solar panels / receivers / switches):");
			inspector.SignalCheckboxes(requiredSignals);
		}


		void MovingBlock::Rotate90(const int quarterTurns)
		{
			// 形を回して、動く向きも一緒に回す（横に動く床 → 縦に動く壁）
			RectGimmick::Rotate90(quarterTurns);
			movePath.Rotate90(quarterTurns);
		}


		void MovingBlock::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("requiredSignals", requiredSignals);
			movePath.Serialize(archive);
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
