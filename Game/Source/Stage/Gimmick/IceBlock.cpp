/**
 * @file IceBlock.cpp
 * @brief 氷
 */
#include "stdafx.h"
#include "IceBlock.h"
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
			/** 溶けた分（元の大きさ）の枠の色 */
			const ImU32 EDITOR_COLOR_MELTED = IM_COL32(190, 235, 255, 80);

			/** 溶けきるまでの時間の下限（0 で割らないように） */
			constexpr float MIN_MELT_TIME = 0.01f;
			/** 光の線分の終わりが、氷の面の上にあるとみなす距離 */
			constexpr float SIDE_EPSILON = 0.5f;
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
		{}


		void IceBlock::Update(const GimmickUpdateContext& context)
		{
			isMeltedThisFrame = false;
			if (IsMelted())
			{
				isLit = false;
				return;
			}
			// 光が当たっている間だけ溶ける（日陰にある間は止まる。溶けた分は戻らない）
			isLit = context.light.IsLit(*this);
			if (!isLit)
			{
				return;
			}
			// 溶け始めたときに、光が当たった面を覚える（そこから縮んでいく）
			if (meltAmount <= 0.0f)
			{
				meltSide = FindLitSide(context.light);
			}
			meltAmount = math::Min(meltAmount + context.deltaTime / math::Max(meltTime, MIN_MELT_TIME), 1.0f);
			if (IsMelted())
			{
				isMeltedThisFrame = true;
			}
		}


		void IceBlock::ResetRuntimeState()
		{
			meltAmount = 0.0f;
			isLit = false;
			isMeltedThisFrame = false;
			meltSide = IceMeltSide::Top;
		}


		math::Rect IceBlock::GetCurrentRect() const
		{
			const float remain = 1.0f - math::Clamp01(meltAmount);
			math::Rect current = rect;
			switch (meltSide)
			{
			case IceMeltSide::Bottom: current.min.y = rect.max.y - rect.Height() * remain; break;
			case IceMeltSide::Left:   current.min.x = rect.max.x - rect.Width() * remain;  break;
			case IceMeltSide::Right:  current.max.x = rect.min.x + rect.Width() * remain;  break;
			case IceMeltSide::Top:
			default:                  current.max.y = rect.min.y + rect.Height() * remain; break;
			}
			return current;
		}


		IceMeltSide IceBlock::FindLitSide(const LightTraceResult& light) const
		{
			// 光の線分の終わりの点が、氷のどの面の上にあるかを数え、一番多い面にする
			const math::Rect current = GetCurrentRect();
			int counts[4] = { 0, 0, 0, 0 };
			for (const LightSegment& segment : light.segments)
			{
				const math::Vec2& p = segment.end;
				const bool isInX = p.x >= current.min.x - SIDE_EPSILON && p.x <= current.max.x + SIDE_EPSILON;
				const bool isInY = p.y >= current.min.y - SIDE_EPSILON && p.y <= current.max.y + SIDE_EPSILON;
				if (!isInX || !isInY)
				{
					continue;
				}
				// 一番近い面（角に当たったときは、上下の面を先に選ぶ）
				const float distances[4] = {
					math::Abs(p.y - current.max.y),	// Top
					math::Abs(p.y - current.min.y),	// Bottom
					math::Abs(p.x - current.min.x),	// Left
					math::Abs(p.x - current.max.x),	// Right
				};
				int side = 0;
				for (int i = 1; i < 4; ++i)
				{
					if (distances[i] < distances[side]) { side = i; }
				}
				if (distances[side] <= SIDE_EPSILON)
				{
					++counts[side];
				}
			}
			int best = 0;
			for (int i = 1; i < 4; ++i)
			{
				if (counts[i] > counts[best]) { best = i; }
			}
			return static_cast<IceMeltSide>(best);
		}


		CollisionKind IceBlock::GetCollision(math::Rect& outRect) const
		{
			if (IsMelted())
			{
				return CollisionKind::None;
			}
			// 溶けて縮んだ分だけ当たり判定も小さくなる（上に乗っていると少しずつ沈む）
			outRect = GetCurrentRect();
			return CollisionKind::Terrain;
		}


		bool IceBlock::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			if (IsMelted())
			{
				return false;
			}
			if (!RayIntersectRect(origin, direction, GetCurrentRect(), outHit.distance))
			{
				return false;
			}
			// 光は氷で止まり、氷が「光が当たった」ことになる（受光器と同じ）
			outHit.response = LightResponse::Absorb;
			return true;
		}


		void IceBlock::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			const bool isKeep = kind == IceKind::Keep;
			const ImU32 fill = isKeep ? EDITOR_COLOR_KEEP_FILL : EDITOR_COLOR_MELT_FILL;
			const ImU32 line = isLit ? EDITOR_COLOR_MELTING : (isKeep ? EDITOR_COLOR_KEEP_LINE : EDITOR_COLOR_MELT_LINE);
			// 元の大きさの枠（溶けた分が分かるように）と、今の大きさ
			if (meltAmount > 0.0f)
			{
				canvas.Rect(rect, 0, EDITOR_COLOR_MELTED);
			}
			if (!IsMelted())
			{
				canvas.Rect(GetCurrentRect(), fill, EditorCanvas::Pick(isSelected, line));
			}
			else if (isSelected)
			{
				canvas.Rect(rect, 0, EditorCanvas::COLOR_SELECTED);
			}
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
			ImGui::DragFloat("Melt Time", &meltTime, 0.05f, MIN_MELT_TIME, 60.0f, "%.2f s");
			ImGui::SetItemTooltip("Seconds of light until it melts away.");
			ImGui::SliderFloat("Melt Amount", &meltAmount, 0.0f, 1.0f, "%.2f");
			ImGui::Text("Lit: %s   Melted: %s", isLit ? "YES" : "no", IsMelted() ? "YES" : "no");
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
		}
	}
}
