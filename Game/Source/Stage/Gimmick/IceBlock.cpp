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
			/** 幅か高さがこれより小さくなったら溶けきったことにする */
			constexpr float MELTED_SIZE = 1.0f;
			/** 面の外向きの向き（IceMeltSide の順：上・下・左・右） */
			const math::Vec2 SIDE_NORMALS[] = { { 0.0f, 1.0f }, { 0.0f, -1.0f }, { -1.0f, 0.0f }, { 1.0f, 0.0f } };
			/** 面の数 */
			constexpr int SIDE_COUNT = static_cast<int>(IceMeltSide::Num);
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
			for (bool& lit : litSides) { lit = false; }
			if (!isLit)
			{
				return;
			}
			// 今フレーム光が当たっている面を調べる（見つからないときは上の面にしておく）
			if (!FindLitSides(context.light))
			{
				litSides[static_cast<int>(IceMeltSide::Top)] = true;
			}
			// 当たっている面ごとに削る。上下の面は「高さ ÷ meltTime」、左右の面は「幅 ÷ meltTime」の速さ
			// → どの面から溶かしても meltTime 秒で溶けきる
			const float time = math::Max(meltTime, MIN_MELT_TIME);
			for (int side = 0; side < SIDE_COUNT; ++side)
			{
				if (!litSides[side])
				{
					continue;
				}
				const bool isVertical = side == static_cast<int>(IceMeltSide::Top) || side == static_cast<int>(IceMeltSide::Bottom);
				const float size = isVertical ? rect.Height() : rect.Width();
				meltedDepth[side] += size / time * context.deltaTime;
			}
			if (IsMelted())
			{
				isMeltedThisFrame = true;
			}
		}


		void IceBlock::ResetRuntimeState()
		{
			for (float& depth : meltedDepth) { depth = 0.0f; }
			for (bool& lit : litSides) { lit = false; }
			isLit = false;
			isMeltedThisFrame = false;
		}


		math::Rect IceBlock::GetCurrentRect() const
		{
			// それぞれの面で溶けた厚さだけ削る（削りすぎて裏返らないようにする）
			math::Rect current = rect;
			current.max.y -= meltedDepth[static_cast<int>(IceMeltSide::Top)];
			current.min.y += meltedDepth[static_cast<int>(IceMeltSide::Bottom)];
			current.min.x += meltedDepth[static_cast<int>(IceMeltSide::Left)];
			current.max.x -= meltedDepth[static_cast<int>(IceMeltSide::Right)];
			if (current.max.y < current.min.y) { current.max.y = current.min.y; }
			if (current.max.x < current.min.x) { current.max.x = current.min.x; }
			return current;
		}


		bool IceBlock::IsMelted() const
		{
			const math::Rect current = GetCurrentRect();
			return current.Width() < MELTED_SIZE || current.Height() < MELTED_SIZE;
		}


		float IceBlock::GetMeltAmount() const
		{
			const float area = rect.Width() * rect.Height();
			if (area <= 0.0f || IsMelted())
			{
				return 1.0f;
			}
			const math::Rect current = GetCurrentRect();
			return math::Clamp01(1.0f - current.Width() * current.Height() / area);
		}


		bool IceBlock::FindLitSides(const LightTraceResult& light)
		{
			// 光の線分の終わりの点が、今の氷のどの面の上にあるかを調べる
			// （その面に外から入ってくる光だけ数える。隣のブロックの反対側に当たった光などは数えない）
			const math::Rect current = GetCurrentRect();
			bool isFound = false;
			for (const LightSegment& segment : light.segments)
			{
				const math::Vec2& p = segment.end;
				const bool isInX = p.x >= current.min.x - SIDE_EPSILON && p.x <= current.max.x + SIDE_EPSILON;
				const bool isInY = p.y >= current.min.y - SIDE_EPSILON && p.y <= current.max.y + SIDE_EPSILON;
				const math::Vec2 delta = segment.end - segment.start;
				if (!isInX || !isInY || delta.LengthSq() <= 0.0f)
				{
					continue;
				}
				const math::Vec2 direction = math::Normalize(delta);
				const float distances[SIDE_COUNT] = {
					math::Abs(p.y - current.max.y),	// 上
					math::Abs(p.y - current.min.y),	// 下
					math::Abs(p.x - current.min.x),	// 左
					math::Abs(p.x - current.max.x),	// 右
				};
				// 点が乗っている面のうち、光が外から入ってくる面（角に当たったときは近いほう）
				int best = -1;
				for (int side = 0; side < SIDE_COUNT; ++side)
				{
					const bool isOnSide = distances[side] <= SIDE_EPSILON;
					const bool isIncoming = math::Dot(direction, SIDE_NORMALS[side]) < 0.0f;
					if (isOnSide && isIncoming && (best < 0 || distances[side] < distances[best]))
					{
						best = side;
					}
				}
				if (best >= 0)
				{
					litSides[best] = true;
					isFound = true;
				}
			}
			return isFound;
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
			if (GetMeltAmount() > 0.0f)
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
			// 実行時の様子（確認用）
			ImGui::Text("Melted: %.0f%%%s", GetMeltAmount() * 100.0f, IsMelted() ? " (gone)" : "");
			ImGui::Text("Lit sides: %s%s%s%s",
				litSides[static_cast<int>(IceMeltSide::Top)] ? "Top " : "",
				litSides[static_cast<int>(IceMeltSide::Bottom)] ? "Bottom " : "",
				litSides[static_cast<int>(IceMeltSide::Left)] ? "Left " : "",
				litSides[static_cast<int>(IceMeltSide::Right)] ? "Right " : "");
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
		}
	}
}
