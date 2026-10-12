/**
 * @file StageDebugOverlay.cpp
 * @brief ステージを3D画面の上に線と面で重ねて描く（デバッグ表示）
 */
#include "stdafx.h"
#include "StageDebugOverlay.h"
#include "Source/Actor/Character/CharacterBase.h"
#include "Source/Actor/Character/CharacterStateMachine.h"
#include "Source/Stage/MirrorInteractor.h"
#include "Source/Stage/Stage.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			// 色（3D画面の上に重ねるので、少し透かす。Windows のマクロ COLOR_〜 とぶつからないよう OVERLAY_ を付けている）
			const ImU32 OVERLAY_COLOR_BLOCK_FILL = IM_COL32(120, 125, 140, 90);
			const ImU32 OVERLAY_COLOR_BLOCK_LINE = IM_COL32(200, 205, 220, 200);
			const ImU32 OVERLAY_COLOR_BACK_MIRROR = IM_COL32(190, 150, 255, 255);
			const ImU32 OVERLAY_COLOR_DOOR_FILL = IM_COL32(170, 110, 60, 200);
			const ImU32 OVERLAY_COLOR_DOOR_LINE = IM_COL32(220, 160, 100, 255);
			const ImU32 OVERLAY_COLOR_PLATFORM_ON = IM_COL32(90, 200, 230, 170);
			const ImU32 OVERLAY_COLOR_PLATFORM_OFF = IM_COL32(90, 200, 230, 110);
			const ImU32 OVERLAY_COLOR_RECEIVER_ON = IM_COL32(255, 235, 90, 255);
			const ImU32 OVERLAY_COLOR_RECEIVER_OFF = IM_COL32(150, 140, 60, 230);
			const ImU32 OVERLAY_COLOR_SOURCE = IM_COL32(255, 160, 60, 255);
			const ImU32 OVERLAY_COLOR_MIRROR_FRONT = IM_COL32(230, 240, 255, 255);
			const ImU32 OVERLAY_COLOR_MIRROR_BACK = IM_COL32(90, 90, 100, 255);
			const ImU32 OVERLAY_COLOR_LIGHT_GLOW = IM_COL32(255, 230, 120, 70);
			const ImU32 OVERLAY_COLOR_LIGHT_CORE = IM_COL32(255, 250, 200, 240);
			const ImU32 OVERLAY_COLOR_CHARACTER = IM_COL32(110, 230, 120, 255);
			const ImU32 OVERLAY_COLOR_PROMPT_BG = IM_COL32(0, 0, 0, 170);
			const ImU32 OVERLAY_COLOR_TEXT = IM_COL32(255, 255, 255, 230);
			const ImU32 OVERLAY_COLOR_SWITCH_OFF = IM_COL32(230, 70, 60, 230);
			const ImU32 OVERLAY_COLOR_SWITCH_ON = IM_COL32(90, 230, 110, 230);
			const ImU32 OVERLAY_COLOR_MIST_AREA = IM_COL32(225, 235, 245, 70);
			const ImU32 OVERLAY_COLOR_MIST_FRONT = IM_COL32(225, 235, 245, 200);
			const ImU32 OVERLAY_COLOR_MIRROR_PLATFORM_FILL = IM_COL32(200, 225, 245, 150);
			const ImU32 OVERLAY_COLOR_MIRROR_PLATFORM_LINE = IM_COL32(200, 225, 245, 230);
			const ImU32 OVERLAY_COLOR_MIRROR_PLATFORM_HIDDEN = IM_COL32(200, 225, 245, 90);
			const ImU32 OVERLAY_COLOR_CHECKPOINT = IM_COL32(110, 230, 120, 120);
			const ImU32 OVERLAY_COLOR_WARP = IM_COL32(170, 120, 255, 200);
			const ImU32 OVERLAY_COLOR_INDICATOR_OFF = IM_COL32(90, 90, 100, 230);
			const ImU32 OVERLAY_COLOR_INDICATOR_ON = IM_COL32(120, 255, 140, 255);
			const ImU32 OVERLAY_COLOR_GOAL = IM_COL32(255, 200, 40, 230);

			/** 鏡に映った側を描くときの濃さ（色の透明度にかける） */
			constexpr float REFLECTION_ALPHA = 0.4f;


			/** 色の透明度に倍率をかける */
			ImU32 ScaleAlpha(const ImU32 color, const float scale)
			{
				const ImU32 alpha = (color >> IM_COL32_A_SHIFT) & 0xFF;
				const ImU32 scaled = static_cast<ImU32>(static_cast<float>(alpha) * scale);
				return (color & ~IM_COL32_A_MASK) | (scaled << IM_COL32_A_SHIFT);
			}


			/**
			 * @brief 決まった奥行き(z)で描くための道具
			 * @details 本物の部屋（z=0）と鏡に映った部屋（z=鏡の奥）を同じ処理で描くために使う
			 */
			struct LayerPainter
			{
				ImDrawList* drawList;
				const StageDebugOverlay::Projector* projector;
				/** 奥行き */
				float z;
				/** 色の濃さ */
				float alpha;

				ImU32 Color(const ImU32 color) const { return color == 0 ? 0 : ScaleAlpha(color, alpha); }

				bool Project(const math::Vec2& world, ImVec2& out) const { return (*projector)(world, z, out); }

				/** 4つの角を変換して四角形を描く */
				void Rect(const math::Rect& rect, const ImU32 fillColor, const ImU32 lineColor, const float thickness = 1.5f) const
				{
					ImVec2 p[4];
					if (!Project({ rect.min.x, rect.min.y }, p[0]) || !Project({ rect.max.x, rect.min.y }, p[1])
						|| !Project({ rect.max.x, rect.max.y }, p[2]) || !Project({ rect.min.x, rect.max.y }, p[3]))
					{
						return;
					}
					if (fillColor != 0) { drawList->AddQuadFilled(p[0], p[1], p[2], p[3], Color(fillColor)); }
					if (lineColor != 0) { drawList->AddQuad(p[0], p[1], p[2], p[3], Color(lineColor), thickness); }
				}

				/** 点線の四角形を描く（見えない物の位置を示す） */
				void DashedRect(const math::Rect& rect, const ImU32 color, const float dash) const
				{
					const math::Vec2 corners[4] = { { rect.min.x, rect.min.y }, { rect.max.x, rect.min.y }, { rect.max.x, rect.max.y }, { rect.min.x, rect.max.y } };
					constexpr int MAX_DASHES_PER_EDGE = 64;
					for (int edge = 0; edge < 4; ++edge)
					{
						const math::Vec2& a = corners[edge];
						const math::Vec2& b = corners[(edge + 1) % 4];
						const float length = (b - a).Length();
						const int count = static_cast<int>(math::Clamp(length / math::Max(dash * 2.0f, 1.0f), 1.0f, static_cast<float>(MAX_DASHES_PER_EDGE)));
						for (int i = 0; i < count; ++i)
						{
							const float t0 = static_cast<float>(i) / static_cast<float>(count);
							const float t1 = (static_cast<float>(i) + 0.5f) / static_cast<float>(count);
							Line(math::Lerp(a, b, t0), math::Lerp(a, b, t1), color, 1.0f);
						}
					}
				}

				/** 線分を描く */
				void Line(const math::Vec2& a, const math::Vec2& b, const ImU32 color, const float thickness) const
				{
					ImVec2 sa;
					ImVec2 sb;
					if (Project(a, sa) && Project(b, sb))
					{
						drawList->AddLine(sa, sb, Color(color), thickness);
					}
				}

				/** 点を描く */
				void Dot(const math::Vec2& p, const float radius, const ImU32 color) const
				{
					ImVec2 s;
					if (Project(p, s))
					{
						drawList->AddCircleFilled(s, radius, Color(color));
					}
				}

				/** 文字を描く（背景付きにもできる） */
				void Label(const math::Vec2& world, const char* text, const ImU32 color, const bool hasBackground) const
				{
					ImVec2 p;
					if (!Project(world, p))
					{
						return;
					}
					const ImVec2 size = ImGui::CalcTextSize(text);
					const ImVec2 pos(p.x - size.x * 0.5f, p.y - size.y - 4.0f);
					if (hasBackground)
					{
						drawList->AddRectFilled(ImVec2(pos.x - 4.0f, pos.y - 2.0f), ImVec2(pos.x + size.x + 4.0f, pos.y + size.y + 2.0f), Color(OVERLAY_COLOR_PROMPT_BG), 4.0f);
					}
					drawList->AddText(pos, Color(color), text);
				}
			};


			/**
			 * @brief ステージのギミックを1枚ぶん描く（本物の部屋 / 鏡に映った部屋 で共通）
			 */
			void DrawLayer(const LayerPainter& painter, const Stage& stage, const bool isFillBlocks, const bool isDrawGimmicks, const bool isDrawLight)
			{
				// 地形ブロック
				for (const StageBlock& block : stage.GetBlocks())
				{
					painter.Rect(block.rect, isFillBlocks ? OVERLAY_COLOR_BLOCK_FILL : 0, OVERLAY_COLOR_BLOCK_LINE);
				}

				if (isDrawGimmicks)
				{
					// 光の足場（出ているときは塗り、消えているときは枠だけ）
					for (const LightPlatform& platform : stage.GetLightPlatforms())
					{
						painter.Rect(platform.rect, platform.IsSolid() ? OVERLAY_COLOR_PLATFORM_ON : 0, OVERLAY_COLOR_PLATFORM_OFF, platform.IsSolid() ? 2.0f : 1.0f);
					}

					// 扉（今の範囲を塗り、閉じたときの範囲を枠で）
					for (const Door& door : stage.GetDoors())
					{
						if (door.IsSolid()) { painter.Rect(door.GetCurrentRect(), OVERLAY_COLOR_DOOR_FILL, 0); }
						painter.Rect(door.closedRect, 0, OVERLAY_COLOR_DOOR_LINE, 1.0f);
					}

					// 受光器
					for (const LightReceiver& receiver : stage.GetLightReceivers())
					{
						painter.Rect(receiver.rect, receiver.isLit ? OVERLAY_COLOR_RECEIVER_ON : OVERLAY_COLOR_RECEIVER_OFF, OVERLAY_COLOR_TEXT, 1.0f);
					}

					// 床スイッチ（押されると緑）
					for (const FloorSwitch& floorSwitch : stage.GetFloorSwitches())
					{
						painter.Rect(floorSwitch.rect, floorSwitch.isPressed ? OVERLAY_COLOR_SWITCH_ON : OVERLAY_COLOR_SWITCH_OFF, 0);
					}

					// 霧の範囲（枠）と霧の先端（出ている間だけ）
					for (const MistEmitter& emitter : stage.GetMistEmitters())
					{
						painter.Rect(emitter.area, 0, OVERLAY_COLOR_MIST_AREA, 1.0f);
						if (emitter.IsActive())
						{
							const float frontY = emitter.GetFrontY();
							painter.Line({ emitter.area.min.x, frontY }, { emitter.area.max.x, frontY }, OVERLAY_COLOR_MIST_FRONT, 2.0f);
						}
					}

					// 鏡の足場（見えるようになったら塗り、見えないうちは点線）
					for (const MirrorPlatform& platform : stage.GetMirrorPlatforms())
					{
						if (platform.isRevealed)
						{
							painter.Rect(platform.rect, OVERLAY_COLOR_MIRROR_PLATFORM_FILL, OVERLAY_COLOR_MIRROR_PLATFORM_LINE, 1.5f);
						}
						else
						{
							painter.DashedRect(platform.rect, OVERLAY_COLOR_MIRROR_PLATFORM_HIDDEN, 8.0f);
						}
					}

					// チェックポイント（点線）
					for (const Checkpoint& checkpoint : stage.GetCheckpoints())
					{
						painter.DashedRect(checkpoint.rect, OVERLAY_COLOR_CHECKPOINT, 10.0f);
					}

					// ワープ（点線と、行き先への線）
					for (const WarpZone& zone : stage.GetWarpZones())
					{
						painter.DashedRect(zone.rect, OVERLAY_COLOR_WARP, 10.0f);
						painter.Line((zone.rect.min + zone.rect.max) * 0.5f, zone.destination, OVERLAY_COLOR_WARP, 1.0f);
					}

					// 合図のランプとゴール
					for (const Indicator& indicator : stage.GetIndicators())
					{
						painter.Rect(indicator.rect, indicator.isOn ? OVERLAY_COLOR_INDICATOR_ON : OVERLAY_COLOR_INDICATOR_OFF, 0);
					}
					for (const Goal& goal : stage.GetGoals())
					{
						painter.Rect(goal.rect, 0, OVERLAY_COLOR_GOAL, 2.0f);
					}
				}

				// 光（太い薄い線の上に細い明るい線を重ねて光らせる）
				if (isDrawLight)
				{
					for (const LightSegment& segment : stage.GetLightResult().segments)
					{
						painter.Line(segment.start, segment.end, OVERLAY_COLOR_LIGHT_GLOW, 9.0f);
						painter.Line(segment.start, segment.end, OVERLAY_COLOR_LIGHT_CORE, 2.5f);
					}
				}

				if (isDrawGimmicks)
				{
					// 反射鏡（表を明るく、裏を暗く。表側に短い線）
					for (const LightMirror& lightMirror : stage.GetLightMirrors())
					{
						const math::Vec2 backOffset = lightMirror.GetNormal() * -3.0f;
						painter.Line(lightMirror.GetStart() + backOffset, lightMirror.GetEnd() + backOffset,
							lightMirror.isDoubleSided ? OVERLAY_COLOR_MIRROR_FRONT : OVERLAY_COLOR_MIRROR_BACK, 4.0f);
						painter.Line(lightMirror.GetStart(), lightMirror.GetEnd(), OVERLAY_COLOR_MIRROR_FRONT, 4.0f);
						painter.Line(lightMirror.center, lightMirror.center + lightMirror.GetNormal() * 15.0f, OVERLAY_COLOR_MIRROR_FRONT, 1.5f);
					}

					// 光源
					for (const LightSource& source : stage.GetLightSources())
					{
						painter.Dot(source.position, 7.0f, source.isOn ? OVERLAY_COLOR_SOURCE : OVERLAY_COLOR_MIRROR_BACK);
					}
				}
			}
		}


		void StageDebugOverlay::Draw(ImDrawList* drawList, const Stage& stage, const MirrorInteractor* interactor, const Projector& projector) const
		{
			if (!isEnabled || drawList == nullptr || !projector)
			{
				return;
			}

			const BackMirror& mirror = stage.GetBackMirror();
			const LayerPainter front = { drawList, &projector, 0.0f, 1.0f };

			// ---- 奥の鏡に映った部屋（先に描いて、手前の部屋の線が上に来るようにする）----
			if (mirror.isEnabled && isDrawReflection)
			{
				const LayerPainter reflected = { drawList, &projector, mirror.ReflectZ(0.0f), REFLECTION_ALPHA };
				DrawLayer(reflected, stage, isFillBlocks, isDrawGimmicks, isDrawLight);
			}

			// ---- 鏡の枠 ----
			if (mirror.isEnabled)
			{
				const LayerPainter frame = { drawList, &projector, mirror.planeZ, 1.0f };
				frame.Rect(mirror.frame, 0, OVERLAY_COLOR_BACK_MIRROR, 2.5f);
			}

			// ---- 手前の部屋 ----
			DrawLayer(front, stage, isFillBlocks, isDrawGimmicks, isDrawLight);

			// ---- キャラクター ----
			if (isDrawCharacters)
			{
				for (const auto& character : m_characters)
				{
					if (character.second == nullptr) { continue; }
					const math::Rect bounds = character.second->GetController().GetBounds();

					// 鏡に映った姿の当たり判定（参考表示）
					if (mirror.isEnabled && isDrawReflection)
					{
						const LayerPainter reflected = { drawList, &projector, mirror.ReflectZ(0.0f), REFLECTION_ALPHA };
						reflected.Rect(bounds, 0, OVERLAY_COLOR_CHARACTER, 1.0f);
					}

					front.Rect(bounds, 0, OVERLAY_COLOR_CHARACTER, 1.5f);
					std::string label = character.first;
					if (const actor::CharacterStateMachine* machine = character.second->GetStateMachine())
					{
						label += " : ";
						label += machine->GetCurrentStateName();
					}
					front.Label({ (bounds.min.x + bounds.max.x) * 0.5f, bounds.max.y + 10.0f }, label.c_str(), OVERLAY_COLOR_CHARACTER, false);
				}
			}

			// ---- 操作できる反射鏡の案内 ----
			if (interactor != nullptr)
			{
				const int index = interactor->GetFocusedMirrorIndex();
				if (index >= 0 && index < static_cast<int>(stage.GetLightMirrors().size()))
				{
					const LightMirror& lightMirror = stage.GetLightMirrors()[index];
					std::string prompt;
					if (lightMirror.isRotatable) { prompt += "[B] Rotate"; }
					if (lightMirror.isMovable) { prompt += prompt.empty() ? "[RB] Drag" : "   [RB] Drag"; }
					if (interactor->IsDragging()) { prompt = "Dragging..."; }
					const math::Rect bounds = lightMirror.GetBounds();
					front.Label({ lightMirror.center.x, bounds.max.y + 25.0f }, prompt.c_str(), OVERLAY_COLOR_TEXT, true);
				}
			}
		}


		void StageDebugOverlay::AddCharacter(const char* label, const actor::CharacterBase* character)
		{
			m_characters.emplace_back(label, character);
		}


		void StageDebugOverlay::DrawSettingsUI()
		{
			ImGui::Checkbox("Overlay", &isEnabled);
			ImGui::SameLine();
			ImGui::Checkbox("Fill Blocks", &isFillBlocks);
			ImGui::Checkbox("Gimmicks", &isDrawGimmicks);
			ImGui::SameLine();
			ImGui::Checkbox("Reflection", &isDrawReflection);
			ImGui::SameLine();
			ImGui::Checkbox("Light", &isDrawLight);
			ImGui::SameLine();
			ImGui::Checkbox("Characters", &isDrawCharacters);
		}
	}
}
