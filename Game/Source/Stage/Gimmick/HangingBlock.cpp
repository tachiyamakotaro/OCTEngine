/**
 * @file HangingBlock.cpp
 * @brief 吊るされた板
 */
#include "stdafx.h"
#include "HangingBlock.h"
#include <algorithm>
#include <cfloat>
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
			const ImU32 EDITOR_COLOR_BLOCK = IM_COL32(160, 110, 60, 255);
			const ImU32 EDITOR_COLOR_ORIGIN = IM_COL32(160, 110, 60, 110);

			/** これより深く落ちたら止める（下に何もないまま落ち続けないように） */
			constexpr float MAX_FALL_DISTANCE = 5000.0f;
			/** 乗る面をさがすときの、わずかなすき間 */
			constexpr float LAND_EPSILON = 0.01f;
		}


		HangingBlock::HangingBlock(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void HangingBlock::Update(const GimmickUpdateContext& context)
		{
			isReleasedThisFrame = false;
			isLandedThisFrame = false;
			if (isLanded)
			{
				return;
			}

			// ロープのどれかが燃え尽きたら落ち始める
			if (!isFalling)
			{
				for (const std::string& rope : ropes)
				{
					if (context.gimmicks.IsSignalOn(rope))
					{
						isFalling = true;
						isReleasedThisFrame = true;
						fallSpeed = 0.0f;
						break;
					}
				}
			}
			if (isFalling)
			{
				Fall(context);
			}
		}


		void HangingBlock::Fall(const GimmickUpdateContext& context)
		{
			fallSpeed = math::Min(fallSpeed + gravity * context.deltaTime, maxFallSpeed);
			const float step = fallSpeed * context.deltaTime;
			const math::Rect current = GetCurrentRect();

			// 今の位置から、下へ step 動かした所までの範囲
			math::Rect sweep = current;
			sweep.min.y -= step;

			// その範囲にある、自分より下の地形・キャラクターの上面のうち、一番高い所
			float landingY = -FLT_MAX;
			bool isOnBody = false;
			std::vector<math::Rect> hits;
			for (const std::unique_ptr<GimmickBase>& gimmick : context.gimmicks.GetAll())
			{
				if (gimmick.get() == this)
				{
					continue;
				}
				hits.clear();
				if (gimmick->QueryCollision(sweep, &hits) == CollisionKind::None)
				{
					continue;
				}
				for (const math::Rect& hit : hits)
				{
					if (hit.max.y <= current.min.y + LAND_EPSILON && hit.max.y > landingY)
					{
						landingY = hit.max.y;
						isOnBody = false;
					}
				}
			}
			for (const math::Rect& body : context.bodies)
			{
				if (sweep.Overlaps(body) && body.max.y <= current.min.y + LAND_EPSILON && body.max.y > landingY)
				{
					landingY = body.max.y;
					isOnBody = true;
				}
			}

			if (landingY > sweep.min.y)
			{
				// 当たった：その上面にぴったり乗せる
				fallDistance = rect.min.y - landingY;
				fallSpeed = 0.0f;
				if (!isOnBody)
				{
					// 地形に乗ったら止まる（キャラクターの頭の上なら、どくまで待つ）
					isFalling = false;
					isLanded = true;
					isLandedThisFrame = true;
				}
				return;
			}

			fallDistance += step;
			if (fallDistance > MAX_FALL_DISTANCE)
			{
				fallDistance = MAX_FALL_DISTANCE;
				isFalling = false;
				isLanded = true;
			}
		}


		void HangingBlock::ResetRuntimeState()
		{
			fallDistance = 0.0f;
			fallSpeed = 0.0f;
			isFalling = false;
			isLanded = false;
			isReleasedThisFrame = false;
			isLandedThisFrame = false;
		}


		CollisionKind HangingBlock::GetCollision(math::Rect& outRect) const
		{
			outRect = GetCurrentRect();
			return CollisionKind::Terrain;
		}


		bool HangingBlock::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (!RayIntersectRect(origin, direction, GetCurrentRect(), outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::Stop;
			return true;
		}


		void HangingBlock::RenameSignal(const std::string& oldName, const std::string& newName)
		{
			for (std::string& rope : ropes)
			{
				if (rope == oldName) { rope = newName; }
			}
		}


		void HangingBlock::RemoveSignal(const std::string& signalName)
		{
			ropes.erase(std::remove(ropes.begin(), ropes.end(), signalName), ropes.end());
		}


		void HangingBlock::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 吊るされている位置を枠で、今の位置を塗りで
			canvas.Rect(rect, 0, EditorCanvas::Pick(isSelected, EDITOR_COLOR_ORIGIN));
			canvas.Rect(GetCurrentRect(), EDITOR_COLOR_BLOCK, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void HangingBlock::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::DragFloat("Gravity", &gravity, 10.0f, 10.0f, 20000.0f, "%.0f");
			ImGui::DragFloat("Max Fall Speed", &maxFallSpeed, 10.0f, 10.0f, 20000.0f, "%.0f");
			ImGui::Text("Fall: %.0f  %s", fallDistance, isLanded ? "LANDED" : (isFalling ? "falling" : "hanging"));
			ImGui::TextUnformatted("Ropes (falls when any one burns out):");
			inspector.SignalCheckboxes(ropes);
		}


		void HangingBlock::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("ropes", ropes);
			archive.Field("gravity", gravity);
			archive.Field("maxFallSpeed", maxFallSpeed);
		}
	}
}
