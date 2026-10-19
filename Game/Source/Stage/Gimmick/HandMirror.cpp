/**
 * @file HandMirror.cpp
 * @brief 手鏡
 */
#include "stdafx.h"
#include "HandMirror.h"
#include <algorithm>
#include <cmath>
#include "GimmickArchive.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_ITEM = IM_COL32(255, 190, 70, 255);
			const ImU32 EDITOR_COLOR_ITEM_FILL = IM_COL32(255, 190, 70, 110);
			const ImU32 EDITOR_COLOR_VIEW = IM_COL32(170, 185, 255, 220);

			/** 視界の角度の範囲（扇形がへこまないように 180 度より小さくする） */
			constexpr float MIN_VIEW_ANGLE_DEG = 1.0f;
			constexpr float MAX_VIEW_ANGLE_DEG = 170.0f;
			/** 矩形との重なりを調べるときに、扇形の中に引く線の数（両端の縁を含む） */
			constexpr int OVERLAP_RAY_COUNT = 9;
			/** 見取り図で弧を描くときの分割数 */
			constexpr int EDITOR_ARC_SEGMENTS = 12;


			/**
			 * @brief 線分 a-b が矩形と重なるか（Liang-Barsky 法）
			 * @details 線分を a + (b - a) * t（t = 0〜1）と表し、矩形の4辺それぞれの内側にいる t の範囲を狭めていく。
			 *          最後に範囲が残っていれば重なっている。
			 */
			bool SegmentOverlapsRect(const math::Vec2& a, const math::Vec2& b, const math::Rect& rect)
			{
				const math::Vec2 d = b - a;
				const float p[4] = { -d.x, d.x, -d.y, d.y };
				const float q[4] = { a.x - rect.min.x, rect.max.x - a.x, a.y - rect.min.y, rect.max.y - a.y };
				float tMin = 0.0f;
				float tMax = 1.0f;
				for (int i = 0; i < 4; ++i)
				{
					if (p[i] == 0.0f)
					{
						// 辺と平行：外側にあれば重ならない
						if (q[i] < 0.0f) { return false; }
						continue;
					}
					const float t = q[i] / p[i];
					if (p[i] < 0.0f)
					{
						// 外から内へ入る
						if (t > tMax) { return false; }
						if (t > tMin) { tMin = t; }
					}
					else
					{
						// 内から外へ出る
						if (t < tMin) { return false; }
						if (t < tMax) { tMax = t; }
					}
				}
				return true;
			}
		}


		/************************************/
		// HandMirrorCone


		bool HandMirrorCone::Contains(const math::Vec2& point) const
		{
			const math::Vec2 toPoint = point - origin;
			const float distanceSq = toPoint.LengthSq();
			if (distanceSq > range * range)
			{
				return false;
			}
			// 頂点そのもの
			if (distanceSq <= 0.0001f)
			{
				return true;
			}
			// 向きとのなす角が、開きの半分以内か（角度ではなく cos で比べる）
			const float cosAngle = math::Dot(toPoint, direction) / std::sqrt(distanceSq);
			return cosAngle >= std::cos(math::DegToRad(halfAngleDeg));
		}


		bool HandMirrorCone::OverlapsRect(const math::Rect& rect) const
		{
			// 1. 矩形の角が視界に入っている
			const math::Vec2 corners[4] = {
				{ rect.min.x, rect.min.y }, { rect.max.x, rect.min.y },
				{ rect.max.x, rect.max.y }, { rect.min.x, rect.max.y },
			};
			for (const math::Vec2& corner : corners)
			{
				if (Contains(corner))
				{
					return true;
				}
			}
			// 2. 頂点（手鏡）が矩形の中にある
			if (origin.x >= rect.min.x && origin.x <= rect.max.x && origin.y >= rect.min.y && origin.y <= rect.max.y)
			{
				return true;
			}
			// 3. 扇形の中に引いた線（両端の縁と、その間）のどれかが矩形を通る
			const float baseDeg = math::RadToDeg(std::atan2(direction.y, direction.x));
			for (int i = 0; i < OVERLAP_RAY_COUNT; ++i)
			{
				const float t = static_cast<float>(i) / static_cast<float>(OVERLAP_RAY_COUNT - 1);
				const float angleDeg = baseDeg + math::Lerp(-halfAngleDeg, halfAngleDeg, t);
				const math::Vec2 end = origin + math::DirectionFromDeg(angleDeg) * range;
				if (SegmentOverlapsRect(origin, end, rect))
				{
					return true;
				}
			}
			return false;
		}


		/************************************/
		// HandMirror


		HandMirror::HandMirror(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void HandMirror::Update(const GimmickUpdateContext& context)
		{
			isCollectedThisFrame = false;
			if (!isCollected)
			{
				// 置き場所に触れたら拾う
				for (const math::Rect& body : context.bodies)
				{
					if (body.Overlaps(rect))
					{
						isCollected = true;
						isCollectedThisFrame = true;
						break;
					}
				}
			}
			UpdateRevealHistory(context.deltaTime);
		}


		void HandMirror::UpdateRevealHistory(const float deltaTime)
		{
			// 古くなった視界を消す（消えたままの時間 + 戻る時間 が過ぎたら、壁は元通り）
			const float lifeTime = math::Max(revealKeepTime + revealRestoreTime, 0.01f);
			for (RevealRecord& record : revealHistory)
			{
				record.age += deltaTime;
			}
			revealHistory.erase(
				std::remove_if(revealHistory.begin(), revealHistory.end(), [lifeTime](const RevealRecord& record) { return record.age >= lifeTime; }),
				revealHistory.end());

			// 構えている間は、決まった間隔で今の視界を覚える（覚えておける数で、消えている時間いっぱいをまかなう間隔）
			// ※ isRaised と手元の位置は、前のフレームの終わりに HandMirrorInteractor が入れた値
			const bool isViewActive = IsViewActive();
			if (isViewActive)
			{
				lastActiveCone = GetCone();
				revealRecordTimer -= deltaTime;
				if (revealRecordTimer <= 0.0f)
				{
					RecordReveal(lastActiveCone);
					revealRecordTimer = lifeTime / static_cast<float>(MAX_REVEAL_COUNT - 1);
				}
			}
			else if (wasViewActive)
			{
				// 下ろした瞬間：最後に当てていた所を覚える（ここから消えたままの時間が始まる）
				RecordReveal(lastActiveCone);
				revealRecordTimer = 0.0f;
			}
			wasViewActive = isViewActive;
		}


		void HandMirror::RecordReveal(const HandMirrorCone& cone)
		{
			RevealRecord record;
			record.cone = cone;
			revealHistory.push_back(record);
			// 今の視界のぶん1つ空けておく（シェーダーに渡せる数に合わせる）
			while (static_cast<int>(revealHistory.size()) > MAX_REVEAL_COUNT - 1)
			{
				revealHistory.erase(revealHistory.begin());
			}
		}


		float HandMirror::GetRevealRate(const float age) const
		{
			if (age <= revealKeepTime)
			{
				return 1.0f;
			}
			return math::Clamp01(1.0f - (age - revealKeepTime) / math::Max(revealRestoreTime, 0.001f));
		}


		void HandMirror::CollectReveals(std::vector<HandMirrorReveal>& out) const
		{
			// 当てている間は、今の視界はずっと消えている
			if (IsViewActive())
			{
				HandMirrorReveal reveal;
				reveal.cone = GetCone();
				reveal.rate = 1.0f;
				out.push_back(reveal);
			}
			// 少し前に当てていた所（新しい順）
			for (auto it = revealHistory.rbegin(); it != revealHistory.rend(); ++it)
			{
				HandMirrorReveal reveal;
				reveal.cone = it->cone;
				reveal.rate = GetRevealRate(it->age);
				out.push_back(reveal);
			}
		}


		void HandMirror::ResetRuntimeState()
		{
			// 拾ったかどうかは残す（死んで再開しても手鏡は持ったまま）
			isCollectedThisFrame = false;
			isRaised = false;
			tiltDeg = 0.0f;
			revealHistory.clear();
			revealRecordTimer = 0.0f;
			wasViewActive = false;
		}


		HandMirrorCone HandMirror::GetCone() const
		{
			HandMirrorCone cone;
			cone.origin = holdPosition;
			cone.direction = math::DirectionFromDeg(GetAimDeg());
			cone.halfAngleDeg = math::Clamp(viewAngleDeg, MIN_VIEW_ANGLE_DEG, MAX_VIEW_ANGLE_DEG) * 0.5f;
			cone.range = math::Max(viewRange, 1.0f);
			return cone;
		}


		void HandMirror::Tilt(const float input, const float deltaTime)
		{
			tiltDeg = math::Clamp(tiltDeg + input * tiltSpeedDeg * deltaTime, -maxTiltDeg, maxTiltDeg);
		}


		void HandMirror::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 拾える範囲（拾ったあとは枠だけ）
			const ImU32 color = EditorCanvas::Pick(isSelected, EDITOR_COLOR_ITEM);
			canvas.Rect(rect, isCollected ? 0 : EDITOR_COLOR_ITEM_FILL, color);
			canvas.Label({ rect.min.x, rect.max.y }, isCollected ? name + " (collected)" : name);

			// 構えている間は視界の扇形
			if (!IsViewActive())
			{
				return;
			}
			const HandMirrorCone cone = GetCone();
			const float startDeg = GetAimDeg() - cone.halfAngleDeg;
			const float endDeg = GetAimDeg() + cone.halfAngleDeg;
			math::Vec2 previous = cone.origin + math::DirectionFromDeg(startDeg) * cone.range;
			canvas.Line(cone.origin, previous, EDITOR_COLOR_VIEW, 1.5f);
			for (int i = 1; i <= EDITOR_ARC_SEGMENTS; ++i)
			{
				const float t = static_cast<float>(i) / static_cast<float>(EDITOR_ARC_SEGMENTS);
				const math::Vec2 point = cone.origin + math::DirectionFromDeg(math::Lerp(startDeg, endDeg, t)) * cone.range;
				canvas.Line(previous, point, EDITOR_COLOR_VIEW, 1.5f);
				previous = point;
			}
			canvas.Line(cone.origin, previous, EDITOR_COLOR_VIEW, 1.5f);
		}


		void HandMirror::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::SetItemTooltip("Pick-up area. The player gets the hand mirror by touching it.");
			ImGui::DragFloat("View Angle", &viewAngleDeg, 1.0f, MIN_VIEW_ANGLE_DEG, MAX_VIEW_ANGLE_DEG, "%.0f deg");
			ImGui::DragFloat("View Range", &viewRange, 5.0f, 10.0f, 5000.0f, "%.0f");
			ImGui::DragFloat("Max Tilt", &maxTiltDeg, 1.0f, 0.0f, 85.0f, "%.0f deg");
			ImGui::DragFloat("Tilt Speed", &tiltSpeedDeg, 1.0f, 0.0f, 720.0f, "%.0f deg/s");
			ImGui::DragFloat("Reveal Keep Time", &revealKeepTime, 0.05f, 0.0f, 30.0f, "%.2f s");
			ImGui::SetItemTooltip("How long fake walls stay gone after the view leaves them.");
			ImGui::DragFloat("Reveal Restore Time", &revealRestoreTime, 0.05f, 0.0f, 30.0f, "%.2f s");
			ImGui::SetItemTooltip("How long fake walls take to come back (dissolve).");
			ImGui::Checkbox("Collected", &isCollected);
			ImGui::SetItemTooltip("ON: the player already has it (for testing). Kept after restart, reset by Reload JSON.");
			ImGui::Text("Raised: %s  Tilt: %.1f deg  Remembered: %d", isRaised ? "YES" : "no", tiltDeg, static_cast<int>(revealHistory.size()));
		}


		void HandMirror::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("viewAngleDeg", viewAngleDeg);
			archive.Field("viewRange", viewRange);
			archive.Field("maxTiltDeg", maxTiltDeg);
			archive.Field("tiltSpeedDeg", tiltSpeedDeg);
			archive.Field("revealKeepTime", revealKeepTime);
			archive.Field("revealRestoreTime", revealRestoreTime);
		}
	}
}
