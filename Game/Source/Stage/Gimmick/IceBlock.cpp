/**
 * @file IceBlock.cpp
 * @brief 氷（光が当たった所から少しずつ穴が開いていく）
 */
#include "stdafx.h"
#include "IceBlock.h"
#include <cfloat>
#include <cmath>
#include <map>
#include <queue>
#include <utility>
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			// 溶かす氷：白っぽい水色 / 守る氷：濃い青（モデルの色と合わせている）
			const ImU32 EDITOR_COLOR_MELT_FILL = IM_COL32(190, 235, 255, 200);
			const ImU32 EDITOR_COLOR_MELT_LINE = IM_COL32(120, 180, 225, 255);
			const ImU32 EDITOR_COLOR_KEEP_FILL = IM_COL32(45, 95, 200, 220);
			const ImU32 EDITOR_COLOR_KEEP_LINE = IM_COL32(200, 225, 255, 255);
			/** 光が当たって溶けている最中の縁の色 */
			const ImU32 EDITOR_COLOR_MELTING = IM_COL32(255, 160, 80, 255);
			/** 元の大きさの枠の色（溶け始めたら薄くする） */
			const ImU32 EDITOR_COLOR_FRAME = IM_COL32(190, 235, 255, 80);

			/** 時間の下限（0 で割らないように） */
			constexpr float MIN_TIME = 0.01f;
			/** マスの大きさの下限 */
			constexpr float MIN_CELL_SIZE = 2.0f;
			/** マスの数の上限（横・縦それぞれ。大きな氷でマスが増えすぎないように） */
			constexpr int MAX_CELLS_PER_SIDE = 200;
			/** 光の線分を氷の範囲で切るとき、範囲を少し広げる量（光の端がちょうど氷の面に乗っているときのため） */
			constexpr float CLIP_MARGIN = 0.5f;
			/**
			 * 熱のたまり方の調整（meltTime を「厚さ 100 を掘り抜くおよその秒数」に合わせるための値）
			 * 光の真上のマスは「1マスの大きさ ÷ 100 × meltTime × この値」秒で溶ける。
			 * 光の先のマスも少しずつ先に温まっていくので、1 より大きくして合わせている（テストで計った値）
			 */
			constexpr float HEAT_TIME_SCALE = 1.6f;


			/** int を範囲に収める */
			int ClampInt(const int value, const int low, const int high)
			{
				return value < low ? low : (value > high ? high : value);
			}


			/** 点と線分の距離 */
			float DistanceToSegment(const math::Vec2& p, const math::Vec2& a, const math::Vec2& b)
			{
				const math::Vec2 ab = b - a;
				const float lengthSq = ab.LengthSq();
				const float t = lengthSq > 0.0f ? math::Clamp01(math::Dot(p - a, ab) / lengthSq) : 0.0f;
				return (p - (a + ab * t)).Length();
			}


			/**
			 * @brief 線分を矩形で切る（Liang-Barsky）
			 * @return 矩形と重なる部分があれば true（outA〜outB がその部分）
			 */
			bool ClipSegment(const math::Vec2& a, const math::Vec2& b, const math::Rect& rect, math::Vec2& outA, math::Vec2& outB)
			{
				float t0 = 0.0f;
				float t1 = 1.0f;
				const math::Vec2 d = b - a;
				const float p[4] = { -d.x, d.x, -d.y, d.y };
				const float q[4] = { a.x - rect.min.x, rect.max.x - a.x, a.y - rect.min.y, rect.max.y - a.y };
				for (int i = 0; i < 4; ++i)
				{
					if (math::Abs(p[i]) < 1e-6f)
					{
						if (q[i] < 0.0f) { return false; }
						continue;
					}
					const float t = q[i] / p[i];
					if (p[i] < 0.0f) { t0 = math::Max(t0, t); }
					else { t1 = math::Min(t1, t); }
					if (t0 > t1) { return false; }
				}
				outA = a + d * t0;
				outB = a + d * t1;
				return true;
			}


			/** レイと矩形の、入る距離と出る距離（当たらなければ false） */
			bool RaySlab(const math::Vec2& origin, const math::Vec2& direction, const math::Rect& rect, float& outEnter, float& outExit)
			{
				float tEnter = -FLT_MAX;
				float tExit = FLT_MAX;
				const float o[2] = { origin.x, origin.y };
				const float d[2] = { direction.x, direction.y };
				const float mn[2] = { rect.min.x, rect.min.y };
				const float mx[2] = { rect.max.x, rect.max.y };
				for (int axis = 0; axis < 2; ++axis)
				{
					if (math::Abs(d[axis]) < 1e-6f)
					{
						if (o[axis] <= mn[axis] || o[axis] >= mx[axis]) { return false; }
						continue;
					}
					float ta = (mn[axis] - o[axis]) / d[axis];
					float tb = (mx[axis] - o[axis]) / d[axis];
					if (ta > tb) { const float tmp = ta; ta = tb; tb = tmp; }
					tEnter = math::Max(tEnter, ta);
					tExit = math::Min(tExit, tb);
					if (tEnter > tExit) { return false; }
				}
				if (tExit < 0.0f) { return false; }
				outEnter = math::Max(tEnter, 0.0f);
				outExit = tExit;
				return true;
			}
		}


		const char* IceKindToString(const IceKind kind)
		{
			return kind == IceKind::Keep ? "Keep" : "Melt";
		}


		bool IceKindFromString(const std::string& text, IceKind& out)
		{
			if (text == "Melt") { out = IceKind::Melt; return true; }
			if (text == "Keep") { out = IceKind::Keep; return true; }
			return false;
		}


		IceBlock::IceBlock(const char* name, const float x, const float y, const float width, const float height, const IceKind initialKind)
			: RectGimmick(name, x, y, width, height)
			, kind(initialKind)
		{
			EnsureGrid();
		}


		// ------------------------------------------------------------
		// マス
		// ------------------------------------------------------------

		void IceBlock::CalcGridSize(int& outColumns, int& outRows) const
		{
			const float size = math::Max(cellSize, MIN_CELL_SIZE);
			const int columns = static_cast<int>(std::ceil(rect.Width() / size - 0.001f));
			const int rows = static_cast<int>(std::ceil(rect.Height() / size - 0.001f));
			outColumns = ClampInt(columns, 1, MAX_CELLS_PER_SIDE);
			outRows = ClampInt(rows, 1, MAX_CELLS_PER_SIDE);
		}


		bool IceBlock::IsGridValid() const
		{
			int columns = 0;
			int rows = 0;
			CalcGridSize(columns, rows);
			return columns == m_columns && rows == m_rows && m_solid.size() == static_cast<size_t>(columns * rows);
		}


		void IceBlock::EnsureGrid()
		{
			if (IsGridValid())
			{
				return;
			}
			// 大きさが変わった（エディタで変えた・読み込んだ）ので、全部のマスが残っている状態から作り直す
			CalcGridSize(m_columns, m_rows);
			const size_t count = static_cast<size_t>(m_columns * m_rows);
			m_solid.assign(count, 1);
			m_heat.assign(count, 0.0f);
			m_solidCount = static_cast<int>(count);
		}


		bool IceBlock::IsCellSolid(const int column, const int row) const
		{
			if (column < 0 || row < 0 || column >= m_columns || row >= m_rows)
			{
				return false;
			}
			if (!IsGridValid())
			{
				// マスがまだ今の大きさに合っていない（大きさを変えた直後）ときは、全部残っていることにする
				return true;
			}
			return m_solid[static_cast<size_t>(CellIndex(column, row))] != 0;
		}


		math::Rect IceBlock::GetCellRect(const int column, const int row) const
		{
			const float w = GetCellWidth();
			const float h = GetCellHeight();
			return math::Rect::FromMinSize(rect.min.x + w * static_cast<float>(column), rect.min.y + h * static_cast<float>(row), w, h);
		}


		bool IceBlock::IsMelted() const
		{
			return IsGridValid() && m_solidCount <= 0;
		}


		float IceBlock::GetMeltAmount() const
		{
			if (!IsGridValid() || m_solid.empty())
			{
				return 0.0f;
			}
			return 1.0f - static_cast<float>(m_solidCount) / static_cast<float>(m_solid.size());
		}


		void IceBlock::CollectSolidRects(std::vector<math::Rect>& out) const
		{
			out.clear();
			if (!IsGridValid())
			{
				out.push_back(rect);
				return;
			}
			// 前の行で作った矩形のうち、まだ上へ伸ばせるもの（左端と右端の列 → out の番号）
			std::map<std::pair<int, int>, size_t> open;
			std::map<std::pair<int, int>, size_t> next;
			const float w = GetCellWidth();
			const float h = GetCellHeight();
			for (int row = 0; row < m_rows; ++row)
			{
				next.clear();
				int column = 0;
				while (column < m_columns)
				{
					if (!IsCellSolid(column, row))
					{
						++column;
						continue;
					}
					// 横に並んだマスを1本にまとめる
					const int start = column;
					while (column < m_columns && IsCellSolid(column, row)) { ++column; }
					const std::pair<int, int> key(start, column);
					const auto found = open.find(key);
					if (found != open.end())
					{
						// すぐ下の行に同じ幅の矩形があれば、上へ伸ばす
						out[found->second].max.y += h;
						next[key] = found->second;
					}
					else
					{
						out.push_back(math::Rect::FromMinSize(rect.min.x + w * static_cast<float>(start), rect.min.y + h * static_cast<float>(row),
							w * static_cast<float>(column - start), h));
						next[key] = out.size() - 1;
					}
				}
				open.swap(next);
			}
		}


		// ------------------------------------------------------------
		// 更新
		// ------------------------------------------------------------

		void IceBlock::Update(const GimmickUpdateContext& context)
		{
			isLit = false;
			isMeltedThisFrame = false;
			isPassableThisFrame = false;
			EnsureGrid();
			if (m_solidCount <= 0)
			{
				return;
			}

			if (isFlowing)
			{
				// 通れる穴が開いたあと：穴から熱が伝わって、残りの氷も流れるように溶けていく
				HeatFromHole(context.deltaTime);
			}
			else
			{
				// 光の通り道のまわりに熱をためる。当たっている時間が長いほど、熱が伝わる半径が広がる
				isLit = HeatFromLight(context);
				if (isLit)
				{
					litTime += context.deltaTime;
				}
			}

			const bool isAnyMelted = MeltHotCells();
			if (isAnyMelted && !isFlowing && HasPassage())
			{
				isFlowing = true;
				isPassableThisFrame = true;
			}
			if (isAnyMelted && m_solidCount <= 0)
			{
				isMeltedThisFrame = true;
			}
		}


		bool IceBlock::HeatFromLight(const GimmickUpdateContext& context)
		{
			const float radius = math::Max(math::Min(heatRadius + heatGrowSpeed * litTime, maxHeatRadius), 1.0f);
			const float w = GetCellWidth();
			const float h = GetCellHeight();
			// 光の真上のマスが溶けるまでの秒数（meltTime は「厚さ 100 を掘り抜くおよその秒数」）
			const float cellTime = math::Max(math::Max(meltTime, MIN_TIME) * (w + h) * 0.5f / 100.0f * HEAT_TIME_SCALE, MIN_TIME);
			const float heatPerSecond = 1.0f / cellTime;

			math::Rect clipRect = rect;
			clipRect.min = clipRect.min - math::Vec2(CLIP_MARGIN, CLIP_MARGIN);
			clipRect.max = clipRect.max + math::Vec2(CLIP_MARGIN, CLIP_MARGIN);

			bool isHeated = false;
			for (const LightSegment& segment : context.light.segments)
			{
				// 光の線分のうち、氷の範囲の中を通る部分（当たった点・掘った穴の中を通る部分）
				math::Vec2 a;
				math::Vec2 b;
				if (!ClipSegment(segment.start, segment.end, clipRect, a, b))
				{
					continue;
				}
				// その部分から radius までのマスに熱をためる（近いほど早く）
				const int minColumn = ClampInt(static_cast<int>(std::floor((math::Min(a.x, b.x) - radius - rect.min.x) / w)), 0, m_columns - 1);
				const int maxColumn = ClampInt(static_cast<int>(std::floor((math::Max(a.x, b.x) + radius - rect.min.x) / w)), 0, m_columns - 1);
				const int minRow = ClampInt(static_cast<int>(std::floor((math::Min(a.y, b.y) - radius - rect.min.y) / h)), 0, m_rows - 1);
				const int maxRow = ClampInt(static_cast<int>(std::floor((math::Max(a.y, b.y) + radius - rect.min.y) / h)), 0, m_rows - 1);
				for (int row = minRow; row <= maxRow; ++row)
				{
					for (int column = minColumn; column <= maxColumn; ++column)
					{
						const size_t index = static_cast<size_t>(CellIndex(column, row));
						if (m_solid[index] == 0)
						{
							continue;
						}
						const math::Vec2 center(rect.min.x + w * (static_cast<float>(column) + 0.5f), rect.min.y + h * (static_cast<float>(row) + 0.5f));
						const float distance = DistanceToSegment(center, a, b);
						if (distance >= radius)
						{
							continue;
						}
						m_heat[index] += heatPerSecond * (1.0f - distance / radius) * context.deltaTime;
						isHeated = true;
					}
				}
			}
			return isHeated;
		}


		void IceBlock::HeatFromHole(const float deltaTime)
		{
			// 溶けたマスのとなり（上下左右）のマスに熱をためる。氷の外は数えない（穴から広がっていくように）
			const float heat = deltaTime / math::Max(flowTime, 0.001f);
			for (int row = 0; row < m_rows; ++row)
			{
				for (int column = 0; column < m_columns; ++column)
				{
					const size_t index = static_cast<size_t>(CellIndex(column, row));
					if (m_solid[index] == 0)
					{
						continue;
					}
					const bool isNextToHole =
						(column > 0 && !IsCellSolid(column - 1, row)) ||
						(column + 1 < m_columns && !IsCellSolid(column + 1, row)) ||
						(row > 0 && !IsCellSolid(column, row - 1)) ||
						(row + 1 < m_rows && !IsCellSolid(column, row + 1));
					if (isNextToHole)
					{
						m_heat[index] += heat;
					}
				}
			}
		}


		bool IceBlock::MeltHotCells()
		{
			bool isAnyMelted = false;
			for (size_t i = 0; i < m_solid.size(); ++i)
			{
				if (m_solid[i] != 0 && m_heat[i] >= 1.0f)
				{
					m_solid[i] = 0;
					--m_solidCount;
					isAnyMelted = true;
				}
			}
			return isAnyMelted;
		}


		bool IceBlock::HasPassage() const
		{
			if (!IsGridValid())
			{
				return false;
			}
			// 通れる箱（passWidth × passHeight）の大きさをマスの数にする
			const bool isVertical = IsVertical();
			int boxColumns = static_cast<int>(std::ceil(math::Max(passWidth, 1.0f) / GetCellWidth() - 0.001f));
			int boxRows = static_cast<int>(std::ceil(math::Max(passHeight, 1.0f) / GetCellHeight() - 0.001f));
			// 氷より大きい箱は、氷の大きさまで（その向きは全部溶けないと通れない）
			if (isVertical) { boxRows = ClampInt(boxRows, 1, m_rows); }
			else { boxColumns = ClampInt(boxColumns, 1, m_columns); }

			// 残っているマスの数の累積（箱の中に残っているマスがあるかを、すぐ調べられるように）
			const int stride = m_columns + 1;
			std::vector<int> sum(static_cast<size_t>((m_columns + 1) * (m_rows + 1)), 0);
			for (int row = 0; row < m_rows; ++row)
			{
				for (int column = 0; column < m_columns; ++column)
				{
					sum[static_cast<size_t>((row + 1) * stride + column + 1)] = (IsCellSolid(column, row) ? 1 : 0)
						+ sum[static_cast<size_t>(row * stride + column + 1)]
						+ sum[static_cast<size_t>((row + 1) * stride + column)]
						- sum[static_cast<size_t>(row * stride + column)];
				}
			}
			// 箱の左下がマス (x, y) にあるとき、箱の中に残っているマスがないか（氷の外は空いている）
			auto isFree = [&](const int x, const int y)
				{
					const int x0 = ClampInt(x, 0, m_columns);
					const int y0 = ClampInt(y, 0, m_rows);
					const int x1 = ClampInt(x + boxColumns, 0, m_columns);
					const int y1 = ClampInt(y + boxRows, 0, m_rows);
					if (x0 >= x1 || y0 >= y1)
					{
						return true;
					}
					const int count = sum[static_cast<size_t>(y1 * stride + x1)] - sum[static_cast<size_t>(y0 * stride + x1)]
						- sum[static_cast<size_t>(y1 * stride + x0)] + sum[static_cast<size_t>(y0 * stride + x0)];
					return count == 0;
				};

			// 箱の左下が動ける範囲
			//   縦長（壁）：左の外（x = -箱の幅）から右の外（x = 列の数）まで。高さは氷の中だけ
			//   横長（床・天井）：下の外から上の外まで。横は氷の中だけ
			const int minX = isVertical ? -boxColumns : 0;
			const int maxX = isVertical ? m_columns : m_columns - boxColumns;
			const int minY = isVertical ? 0 : -boxRows;
			const int maxY = isVertical ? m_rows - boxRows : m_rows;
			if (maxX < minX || maxY < minY)
			{
				return false;
			}
			const int width = maxX - minX + 1;
			const int height = maxY - minY + 1;
			std::vector<std::uint8_t> visited(static_cast<size_t>(width * height), 0);
			std::queue<std::pair<int, int>> queue;
			// 片側の外から始める（外は空いているので必ず置ける）
			const int startCount = isVertical ? height : width;
			for (int i = 0; i < startCount; ++i)
			{
				const int x = isVertical ? minX : minX + i;
				const int y = isVertical ? minY + i : minY;
				visited[static_cast<size_t>((y - minY) * width + (x - minX))] = 1;
				queue.push({ x, y });
			}
			const int dx[4] = { 1, -1, 0, 0 };
			const int dy[4] = { 0, 0, 1, -1 };
			while (!queue.empty())
			{
				const std::pair<int, int> position = queue.front();
				queue.pop();
				// 反対側の外まで行けたら通れる
				if ((isVertical && position.first == maxX) || (!isVertical && position.second == maxY))
				{
					return true;
				}
				for (int i = 0; i < 4; ++i)
				{
					const int x = position.first + dx[i];
					const int y = position.second + dy[i];
					if (x < minX || x > maxX || y < minY || y > maxY)
					{
						continue;
					}
					const size_t index = static_cast<size_t>((y - minY) * width + (x - minX));
					if (visited[index] != 0 || !isFree(x, y))
					{
						continue;
					}
					visited[index] = 1;
					queue.push({ x, y });
				}
			}
			return false;
		}


		void IceBlock::ResetRuntimeState()
		{
			// 全部のマスが残っている状態に戻す
			m_columns = 0;
			m_rows = 0;
			m_solid.clear();
			EnsureGrid();
			isLit = false;
			litTime = 0.0f;
			isFlowing = false;
			isPassableThisFrame = false;
			isMeltedThisFrame = false;
		}


		// ------------------------------------------------------------
		// 当たり判定・光
		// ------------------------------------------------------------

		CollisionKind IceBlock::GetCollision(math::Rect& outRect) const
		{
			if (IsMelted())
			{
				return CollisionKind::None;
			}
			// 残っているマス全体を囲む矩形
			std::vector<math::Rect> solids;
			CollectSolidRects(solids);
			if (solids.empty())
			{
				return CollisionKind::None;
			}
			math::Rect bounds = solids[0];
			for (const math::Rect& solid : solids)
			{
				bounds.min.x = math::Min(bounds.min.x, solid.min.x);
				bounds.min.y = math::Min(bounds.min.y, solid.min.y);
				bounds.max.x = math::Max(bounds.max.x, solid.max.x);
				bounds.max.y = math::Max(bounds.max.y, solid.max.y);
			}
			outRect = bounds;
			return CollisionKind::Terrain;
		}


		CollisionKind IceBlock::QueryCollision(const math::Rect& box, std::vector<math::Rect>* out) const
		{
			if (!box.Overlaps(rect) || IsMelted())
			{
				return CollisionKind::None;
			}
			if (!IsGridValid())
			{
				if (out != nullptr) { out->push_back(rect); }
				return CollisionKind::Terrain;
			}
			// box と重なる行・列だけ調べ、横に並んだマスは1本の矩形にまとめて足す
			const float w = GetCellWidth();
			const float h = GetCellHeight();
			const int minColumn = ClampInt(static_cast<int>(std::floor((box.min.x - rect.min.x) / w)), 0, m_columns - 1);
			const int maxColumn = ClampInt(static_cast<int>(std::floor((box.max.x - rect.min.x) / w)), 0, m_columns - 1);
			const int minRow = ClampInt(static_cast<int>(std::floor((box.min.y - rect.min.y) / h)), 0, m_rows - 1);
			const int maxRow = ClampInt(static_cast<int>(std::floor((box.max.y - rect.min.y) / h)), 0, m_rows - 1);
			bool isHit = false;
			for (int row = minRow; row <= maxRow; ++row)
			{
				int column = minColumn;
				while (column <= maxColumn)
				{
					if (!IsCellSolid(column, row))
					{
						++column;
						continue;
					}
					const int start = column;
					while (column <= maxColumn && IsCellSolid(column, row)) { ++column; }
					const math::Rect run = math::Rect::FromMinSize(rect.min.x + w * static_cast<float>(start), rect.min.y + h * static_cast<float>(row),
						w * static_cast<float>(column - start), h);
					if (!box.Overlaps(run))
					{
						continue;
					}
					isHit = true;
					if (out == nullptr)
					{
						return CollisionKind::Terrain;
					}
					out->push_back(run);
				}
			}
			return isHit ? CollisionKind::Terrain : CollisionKind::None;
		}


		bool IceBlock::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (IsMelted())
			{
				return false;
			}
			if (!IsGridValid())
			{
				if (!RayIntersectRect(origin, direction, rect, outHit.distance)) { return false; }
				outHit.response = LightResponse::Absorb;
				return true;
			}
			float tEnter = 0.0f;
			float tExit = 0.0f;
			if (!RaySlab(origin, direction, rect, tEnter, tExit))
			{
				return false;
			}
			// 光が通る順にマスをたどる（Amanatides & Woo の方法）。最初に残っているマスに入る所で止まる
			const float w = GetCellWidth();
			const float h = GetCellHeight();
			const math::Vec2 start = origin + direction * (tEnter + 0.001f);
			int column = ClampInt(static_cast<int>(std::floor((start.x - rect.min.x) / w)), 0, m_columns - 1);
			int row = ClampInt(static_cast<int>(std::floor((start.y - rect.min.y) / h)), 0, m_rows - 1);

			const int stepX = direction.x > 0.0f ? 1 : (direction.x < 0.0f ? -1 : 0);
			const int stepY = direction.y > 0.0f ? 1 : (direction.y < 0.0f ? -1 : 0);
			const float nextX = rect.min.x + w * static_cast<float>(column + (stepX > 0 ? 1 : 0));
			const float nextY = rect.min.y + h * static_cast<float>(row + (stepY > 0 ? 1 : 0));
			float tMaxX = stepX != 0 ? (nextX - origin.x) / direction.x : FLT_MAX;
			float tMaxY = stepY != 0 ? (nextY - origin.y) / direction.y : FLT_MAX;
			const float tDeltaX = stepX != 0 ? w / math::Abs(direction.x) : FLT_MAX;
			const float tDeltaY = stepY != 0 ? h / math::Abs(direction.y) : FLT_MAX;

			float tCell = tEnter;
			while (column >= 0 && column < m_columns && row >= 0 && row < m_rows && tCell <= tExit)
			{
				if (IsCellSolid(column, row))
				{
					// 光は氷で止まり、氷が「光が当たった」ことになる
					outHit.distance = tCell;
					outHit.response = LightResponse::Absorb;
					return true;
				}
				if (tMaxX < tMaxY)
				{
					tCell = tMaxX;
					tMaxX += tDeltaX;
					column += stepX;
				}
				else
				{
					tCell = tMaxY;
					tMaxY += tDeltaY;
					row += stepY;
				}
			}
			// 穴を通り抜けた
			return false;
		}


		// ------------------------------------------------------------
		// エディタ・ファイル
		// ------------------------------------------------------------

		void IceBlock::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			const bool isKeep = kind == IceKind::Keep;
			const ImU32 fill = isKeep ? EDITOR_COLOR_KEEP_FILL : EDITOR_COLOR_MELT_FILL;
			const ImU32 line = isLit ? EDITOR_COLOR_MELTING : (isKeep ? EDITOR_COLOR_KEEP_LINE : EDITOR_COLOR_MELT_LINE);
			// 残っているマス（まとめた矩形）と、元の大きさの枠
			std::vector<math::Rect> solids;
			CollectSolidRects(solids);
			for (const math::Rect& solid : solids)
			{
				canvas.Rect(solid, fill, 0);
			}
			canvas.Rect(rect, 0, EditorCanvas::Pick(isSelected, GetMeltAmount() > 0.0f && !isLit ? EDITOR_COLOR_FRAME : line));
			canvas.Label({ rect.min.x, rect.max.y }, name + (isKeep ? " (keep)" : " (melt)"));
		}


		void IceBlock::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			int kindIndex = kind == IceKind::Keep ? 1 : 0;
			if (ImGui::Combo("Kind", &kindIndex, "Melt (blocks the way)\0Keep (foothold)\0"))
			{
				kind = kindIndex == 1 ? IceKind::Keep : IceKind::Melt;
			}
			ImGui::SetItemTooltip("Melt: melt it with light to open the way.\nKeep: a foothold. Protect it with shade.\n(Both melt in light. Only the look is different.)");
			ImGui::DragFloat("Melt Time", &meltTime, 0.05f, MIN_TIME, 60.0f, "%.2f s");
			ImGui::SetItemTooltip("About how many seconds a beam takes to dig through 100 of ice.");
			if (ImGui::DragFloat("Cell Size", &cellSize, 0.5f, MIN_CELL_SIZE, 100.0f, "%.1f"))
			{
				// マスの大きさを変えたら、全部のマスが残っている状態から作り直す
				EnsureGrid();
			}
			ImGui::SetItemTooltip("Size of one ice cell. Smaller = finer holes, but heavier.");
			ImGui::DragFloat("Heat Radius", &heatRadius, 0.5f, 1.0f, 1000.0f, "%.0f");
			ImGui::SetItemTooltip("Cells closer than this to the beam get warm (when the light starts).");
			ImGui::DragFloat("Heat Grow Speed", &heatGrowSpeed, 0.5f, 0.0f, 1000.0f, "%.0f /s");
			ImGui::SetItemTooltip("How fast the heat radius grows while lit (the hole gets wider).");
			ImGui::DragFloat("Max Heat Radius", &maxHeatRadius, 0.5f, 1.0f, 2000.0f, "%.0f");
			ImGui::DragFloat("Flow Time", &flowTime, 0.005f, 0.001f, 2.0f, "%.3f s");
			ImGui::SetItemTooltip("After a passable hole opens, seconds for the melting to spread to the next cell.");
			ImGui::DragFloat("Pass Width", &passWidth, 1.0f, 1.0f, 1000.0f, "%.0f");
			ImGui::DragFloat("Pass Height", &passHeight, 1.0f, 1.0f, 1000.0f, "%.0f");
			ImGui::SetItemTooltip("Size of the hole the player needs. A tall ice checks left-right, a wide ice checks up-down.");
			ImGui::Text("Cells: %d x %d   Melted: %.0f%%", m_columns, m_rows, GetMeltAmount() * 100.0f);
			ImGui::Text("Lit: %s   Passable: %s", isLit ? "YES" : "no", isFlowing ? "YES (melting away)" : "no");
			if (ImGui::SmallButton("Refreeze"))
			{
				// 溶けた分を元に戻す（Reset Runtime と同じことを、この氷だけに）
				ResetRuntimeState();
			}
		}


		void IceBlock::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			// 種類は "Melt" / "Keep" の文字で書く（JSON を手で読んでも分かるように）
			std::string kindText = IceKindToString(kind);
			archive.Field("kind", kindText);
			if (archive.IsReading())
			{
				// 知らない文字なら今の種類のまま
				IceKindFromString(kindText, kind);
			}
			archive.Field("meltTime", meltTime);
			archive.Field("cellSize", cellSize);
			archive.Field("heatRadius", heatRadius);
			archive.Field("heatGrowSpeed", heatGrowSpeed);
			archive.Field("maxHeatRadius", maxHeatRadius);
			archive.Field("flowTime", flowTime);
			archive.Field("passWidth", passWidth);
			archive.Field("passHeight", passHeight);
			if (archive.IsReading())
			{
				// 読み込んだ大きさに合わせてマスを作り直す
				EnsureGrid();
			}
		}
	}
}
