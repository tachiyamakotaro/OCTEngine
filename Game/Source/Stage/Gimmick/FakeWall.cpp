/**
 * @file FakeWall.cpp
 * @brief 偽物の壁
 */
#include "stdafx.h"
#include "FakeWall.h"
#include <cmath>
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
			/** 地形ブロックと同じ色で塗り、枠だけ紫にして見分ける */
			const ImU32 EDITOR_COLOR_FILL = IM_COL32(120, 125, 140, 255);
			const ImU32 EDITOR_COLOR_FILL_PASSABLE = IM_COL32(120, 125, 140, 50);
			const ImU32 EDITOR_COLOR_LINE = IM_COL32(190, 120, 255, 255);

			/** マスの大きさの下限（小さすぎるとマスが増えすぎる） */
			constexpr float MIN_CELL_SIZE = 2.0f;
			/** 光の線分の終わりが、壁からこれだけ手前でも「当たった」とみなす（計算の誤差のぶん） */
			constexpr float LIGHT_TOUCH_EPSILON = 1.0f;

			/** JSON に書く、光で消える形の名前 */
			const char* const LIGHT_MODE_NAMES[] = { "Whole", "Hole", "Band" };
			constexpr int LIGHT_MODE_COUNT = 3;


			/** @brief 点と線分の距離 */
			float DistanceToSegment(const math::Vec2& p, const math::Vec2& a, const math::Vec2& b)
			{
				const math::Vec2 ab = b - a;
				const float lengthSq = ab.LengthSq();
				const float t = lengthSq > 0.0001f ? math::Clamp01(math::Dot(p - a, ab) / lengthSq) : 0.0f;
				return (p - (a + ab * t)).Length();
			}


			/** @brief 壁の端からの距離が、何番目のマスか（0 〜 count-1 に収める） */
			int CellIndex(const float offset, const float cell, const int count)
			{
				const int index = static_cast<int>(std::floor(offset / cell));
				if (index < 0) { return 0; }
				if (index > count - 1) { return count - 1; }
				return index;
			}


			/**
			 * @brief 光が矩形の中を通る範囲（入る距離と出る距離）
			 * @return 矩形を通るなら true
			 */
			bool RayRectRange(const math::Vec2& origin, const math::Vec2& direction, const math::Rect& rect, float& outEnter, float& outExit)
			{
				float enter = 0.0f;
				float exit = 1.0e30f;
				const float o[2] = { origin.x, origin.y };
				const float d[2] = { direction.x, direction.y };
				const float lo[2] = { rect.min.x, rect.min.y };
				const float hi[2] = { rect.max.x, rect.max.y };
				for (int axis = 0; axis < 2; ++axis)
				{
					if (std::fabs(d[axis]) < 1.0e-8f)
					{
						// この軸に平行：範囲の外なら通らない
						if (o[axis] < lo[axis] || o[axis] > hi[axis])
						{
							return false;
						}
						continue;
					}
					float t0 = (lo[axis] - o[axis]) / d[axis];
					float t1 = (hi[axis] - o[axis]) / d[axis];
					if (t0 > t1)
					{
						const float t = t0;
						t0 = t1;
						t1 = t;
					}
					enter = math::Max(enter, t0);
					exit = math::Min(exit, t1);
					if (enter > exit)
					{
						return false;
					}
				}
				outEnter = enter;
				outExit = exit;
				return true;
			}
		}


		FakeWall::FakeWall(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void FakeWall::Update(const GimmickUpdateContext& context)
		{
			// 消えている範囲（今の視界と、少し前に当てていた所）を集める
			reveals.clear();
			for (const HandMirror& mirror : context.gimmicks.View<HandMirror>())
			{
				mirror.CollectReveals(reveals);
			}
			// 光（反射した光）が当たっているか、どこに穴が開くか
			UpdateLightReveal(context.light);

			isPassable = IsPassableOverlap(rect) || isLitByLight;

			// 消えていない所に体が重なっていたら、元に戻った壁の中にいる
			// （キャラクターは消えていないマスには入れないので、重なるのは、中にいる間に壁が戻ったときだけ）
			// 体と同じ高さの所を、壁の端から端まで空けておく → 横に歩けば壁から出られる（上下には動けない）
			// 壁から出たら、次のフレームから元に戻る
			holdOpenAreas.clear();
			for (const math::Rect& body : context.bodies)
			{
				if (body.Overlaps(rect) && CollectSolidCells(body, nullptr, false))
				{
					holdOpenAreas.push_back({ { rect.min.x, body.min.y }, { rect.max.x, body.max.y } });
				}
			}
		}


		void FakeWall::UpdateLightReveal(const LightTraceResult& light)
		{
			isLitByLight = false;
			lightHoles.clear();
			for (const LightSegment& segment : light.segments)
			{
				// 光源からまっすぐ来た光（bounce = 0）は数えない
				if (isReflectedLightOnly && segment.bounce < 1)
				{
					continue;
				}
				const math::Vec2 delta = segment.end - segment.start;
				const float length = delta.Length();
				if (length < 0.0001f)
				{
					continue;
				}
				const math::Vec2 direction = delta * (1.0f / length);

				// 光の線をのばして壁を通る区間を出し、線分がそこまで届いているか調べる
				// （消えていない壁では光は壁の表面で止まるので、線分の終わり＝壁の表面）
				// 太い光は、太さの半分だけ壁を大きくして調べる（光の真ん中が外れていても、光の端が壁にかかっていれば当たり）
				const float halfWidth = math::Max(segment.width * 0.5f, 0.0f);
				math::Rect hitArea = rect;
				hitArea.min = hitArea.min - math::Vec2(halfWidth, halfWidth);
				hitArea.max = hitArea.max + math::Vec2(halfWidth, halfWidth);
				float enter = 0.0f;
				float exit = 0.0f;
				if (!RayRectRange(segment.start, direction, hitArea, enter, exit) || enter > length + LIGHT_TOUCH_EPSILON)
				{
					continue;
				}
				isLitByLight = true;

				// 光が壁に入る点と出る点（線分の終わりではなく、壁の反対側まで）
				const math::Vec2 entryPoint = segment.start + direction * enter;
				const math::Vec2 exitPoint = segment.start + direction * exit;
				if (static_cast<int>(lightHoles.size()) >= MAX_LIGHT_HOLES)
				{
					continue;
				}
				switch (lightMode)
				{
				case FakeWallLightMode::Hole:
				{
					// 光が壁を通る道の真ん中を中心に丸く（壁の厚みの半分より半径が大きければ、向こう側まで抜ける）
					const math::Vec2 center = (entryPoint + exitPoint) * 0.5f;
					lightHoles.push_back({ center, center, math::Max(lightHoleRadius, 1.0f) });
					break;
				}
				case FakeWallLightMode::Band:
					// 光の道に沿って帯の形に（太い光なら、光が当たっている所がそのまま消える）
					lightHoles.push_back({ entryPoint, exitPoint, math::Max(math::Max(lightBandRadius, halfWidth), 1.0f) });
					break;
				case FakeWallLightMode::Whole:
				default:
					// 壁ごと消えるので、穴はいらない
					break;
				}
			}
		}


		void FakeWall::ResetRuntimeState()
		{
			isPassable = false;
			reveals.clear();
			holdOpenAreas.clear();
			isLitByLight = false;
			lightHoles.clear();
		}


		CollisionKind FakeWall::GetCollision(math::Rect& outRect) const
		{
			if (isPassable || !holdOpenAreas.empty())
			{
				return CollisionKind::None;
			}
			outRect = rect;
			return CollisionKind::Terrain;
		}


		CollisionKind FakeWall::QueryCollision(const math::Rect& box, std::vector<math::Rect>* out) const
		{
			if (!box.Overlaps(rect) || IsWhollyRevealed())
			{
				return CollisionKind::None;
			}

			// どこも消えていなければ、今まで通り壁全体で1つ（マスに分けなくてよい）
			if (!isPassable && holdOpenAreas.empty())
			{
				if (out != nullptr)
				{
					out->push_back(rect);
				}
				return CollisionKind::Terrain;
			}

			return CollectSolidCells(box, out, true) ? CollisionKind::Terrain : CollisionKind::None;
		}


		bool FakeWall::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			float enter = 0.0f;
			float exit = 0.0f;
			if (!RayRectRange(origin, direction, rect, enter, exit))
			{
				return false;
			}

			// 壁の中を少しずつ進んで、消えていない所に当たったら止まる
			const float step = GetCellSize() * 0.5f;
			for (float t = enter; t <= exit; t += step)
			{
				if (!IsPassableAt(origin + direction * t))
				{
					outHit.distance = t;
					outHit.response = LightResponse::Stop;
					return true;
				}
			}
			// 全部消えている所を通った
			return false;
		}


		void FakeWall::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			if (!isPassable && holdOpenAreas.empty())
			{
				canvas.Rect(rect, EDITOR_COLOR_FILL, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE));
			}
			else
			{
				// 消えている所があるときは、全体を薄く塗って、当たり判定が残っているマスだけ濃く塗る
				canvas.Rect(rect, EDITOR_COLOR_FILL_PASSABLE, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE));
				std::vector<math::Rect> cells;
				CollectSolidCells(rect, &cells, true);
				for (const math::Rect& cell : cells)
				{
					canvas.Rect(cell, EDITOR_COLOR_FILL, 0);
				}
			}
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void FakeWall::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::SetItemTooltip("Looks like a normal block. Disappears inside the hand mirror's view.");
			ImGui::SliderFloat("Pass Reveal Rate", &passRevealRate, 0.0f, 1.0f, "%.2f");
			ImGui::SetItemTooltip("Passable while the wall is at least this much gone (1 = fully gone).");
			ImGui::DragFloat("Collision Cell Size", &collisionCellSize, 0.5f, MIN_CELL_SIZE, 100.0f, "%.1f");
			ImGui::SetItemTooltip("Size of the collision cells. Smaller = closer to the look, a little heavier.");
			ImGui::Separator();
			int modeIndex = static_cast<int>(lightMode);
			if (ImGui::Combo("Light Mode", &modeIndex, LIGHT_MODE_NAMES, LIGHT_MODE_COUNT))
			{
				lightMode = static_cast<FakeWallLightMode>(modeIndex);
			}
			ImGui::SetItemTooltip("Whole: the whole wall disappears / Hole: a round hole / Band: along the light.");
			ImGui::Checkbox("Reflected Light Only", &isReflectedLightOnly);
			ImGui::SetItemTooltip("ON: only light bounced by a mirror opens the wall.");
			ImGui::DragFloat("Hole Radius", &lightHoleRadius, 1.0f, 1.0f, 1000.0f, "%.0f");
			ImGui::DragFloat("Band Radius", &lightBandRadius, 1.0f, 1.0f, 1000.0f, "%.0f");
			ImGui::Text("Lit: %s  Holes: %d", isLitByLight ? "YES" : "no", static_cast<int>(lightHoles.size()));
			ImGui::Text("Passable: %s  Reveals: %d  Hold: %d", isPassable ? "YES" : "no",
				static_cast<int>(reveals.size()), static_cast<int>(holdOpenAreas.size()));
		}


		void FakeWall::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("passRevealRate", passRevealRate);
			archive.Field("collisionCellSize", collisionCellSize);

			// 光で消える形は名前で書く（"Whole" / "Hole" / "Band"）。知らない名前なら今のまま
			std::string modeName = LIGHT_MODE_NAMES[static_cast<int>(lightMode)];
			archive.Field("lightMode", modeName);
			if (archive.IsReading())
			{
				for (int i = 0; i < LIGHT_MODE_COUNT; ++i)
				{
					if (modeName == LIGHT_MODE_NAMES[i])
					{
						lightMode = static_cast<FakeWallLightMode>(i);
					}
				}
			}
			archive.Field("isReflectedLightOnly", isReflectedLightOnly);
			archive.Field("lightHoleRadius", lightHoleRadius);
			archive.Field("lightBandRadius", lightBandRadius);
		}


		bool FakeWall::IsPassableAt(const math::Vec2& point) const
		{
			// 光で壁ごと消えている
			if (IsWhollyRevealed())
			{
				return true;
			}
			// 光で開いた穴
			for (const FakeWallLightHole& hole : lightHoles)
			{
				if (DistanceToSegment(point, hole.a, hole.b) <= hole.radius)
				{
					return true;
				}
			}
			// 手鏡の視界
			for (const HandMirrorReveal& reveal : reveals)
			{
				if (reveal.rate >= passRevealRate && reveal.cone.Contains(point))
				{
					return true;
				}
			}
			return false;
		}


		bool FakeWall::IsPassableOverlap(const math::Rect& area) const
		{
			for (const HandMirrorReveal& reveal : reveals)
			{
				if (reveal.rate >= passRevealRate && reveal.cone.OverlapsRect(area))
				{
					return true;
				}
			}
			return false;
		}


		float FakeWall::GetCellSize() const
		{
			return math::Max(collisionCellSize, MIN_CELL_SIZE);
		}


		bool FakeWall::CollectSolidCells(const math::Rect& box, std::vector<math::Rect>* out, const bool isUseHold) const
		{
			const float cell = GetCellSize();
			const int columnCount = static_cast<int>(std::ceil(rect.Width() / cell));
			const int rowCount = static_cast<int>(std::ceil(rect.Height() / cell));
			if (columnCount <= 0 || rowCount <= 0)
			{
				return false;
			}

			// box と重なるマスの範囲だけ調べる
			const int firstColumn = CellIndex(box.min.x - rect.min.x, cell, columnCount);
			const int lastColumn = CellIndex(box.max.x - rect.min.x, cell, columnCount);
			const int firstRow = CellIndex(box.min.y - rect.min.y, cell, rowCount);
			const int lastRow = CellIndex(box.max.y - rect.min.y, cell, rowCount);

			bool isFound = false;
			for (int row = firstRow; row <= lastRow; ++row)
			{
				const float cellMinY = rect.min.y + static_cast<float>(row) * cell;
				const float cellMaxY = math::Min(cellMinY + cell, rect.max.y);

				// 横に続いている、消えていないマスを1つにまとめる
				bool isInRun = false;
				float runMinX = 0.0f;
				float runMaxX = 0.0f;
				for (int column = firstColumn; column <= lastColumn; ++column)
				{
					const float cellMinX = rect.min.x + static_cast<float>(column) * cell;
					const float cellMaxX = math::Min(cellMinX + cell, rect.max.x);
					const math::Rect cellRect{ { cellMinX, cellMinY }, { cellMaxX, cellMaxY } };

					// ぎりぎり触れているだけのマスは入れない（キャラクターの押し出しがずれないように）
					bool isSolid = box.Overlaps(cellRect);
					if (isSolid)
					{
						const math::Vec2 center{ (cellMinX + cellMaxX) * 0.5f, (cellMinY + cellMaxY) * 0.5f };
						isSolid = !IsPassableAt(center);
					}
					if (isSolid && isUseHold)
					{
						for (const math::Rect& hold : holdOpenAreas)
						{
							if (hold.Overlaps(cellRect))
							{
								isSolid = false;
								break;
							}
						}
					}

					if (isSolid)
					{
						if (out == nullptr)
						{
							return true;
						}
						if (!isInRun)
						{
							isInRun = true;
							runMinX = cellMinX;
						}
						runMaxX = cellMaxX;
						isFound = true;
					}
					else if (isInRun)
					{
						out->push_back({ { runMinX, cellMinY }, { runMaxX, cellMaxY } });
						isInRun = false;
					}
				}
				if (isInRun)
				{
					out->push_back({ { runMinX, cellMinY }, { runMaxX, cellMaxY } });
				}
			}
			return isFound;
		}
	}
}
