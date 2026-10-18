/**
 * @file StageEditor.cpp
 * @brief ImGui のステージエディタ
 */
#include "stdafx.h"
#include "Source/Actor/Character/CharacterBase.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/StageLoader.h"
#include "StageEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>


namespace app
{
	namespace stage
	{
		namespace
		{
			/** 一覧・プロパティの欄の幅 */
			constexpr float LEFT_PANE_WIDTH = 320.0f;
			/** 線を選択できる距離（ピクセル） */
			constexpr float PICK_DISTANCE_PX = 8.0f;
			/** 拡大率の範囲 */
			constexpr float MIN_ZOOM = 0.05f;
			constexpr float MAX_ZOOM = 4.0f;
			/** 複製したときにずらす量 */
			constexpr float DUPLICATE_OFFSET = 50.0f;
			/** 矢印の長さと先端の大きさ（ピクセル） */
			constexpr float GIZMO_LENGTH_PX = 60.0f;
			constexpr float GIZMO_HEAD_PX = 10.0f;
			/** 覚えておく履歴の数 */
			constexpr size_t MAX_HISTORY = 100;

			const ImU32 EDITOR_COLOR_GIZMO_X = IM_COL32(235, 70, 70, 255);
			const ImU32 EDITOR_COLOR_GIZMO_Y = IM_COL32(90, 220, 90, 255);
			const ImU32 EDITOR_COLOR_GIZMO_HOT = IM_COL32(255, 230, 60, 255);

			// 色（COLOR_BACKGROUND などは Windows のヘッダー(winuser.h)がマクロで使っているので、EDITOR_ を付けている）
			// ギミックの色は、それぞれのギミックのクラス（DrawEditor）が持っている
			const ImU32 EDITOR_COLOR_BACKGROUND = IM_COL32(28, 30, 36, 255);
			const ImU32 EDITOR_COLOR_GRID = IM_COL32(255, 255, 255, 18);
			const ImU32 EDITOR_COLOR_AXIS = IM_COL32(255, 255, 255, 50);
			const ImU32 EDITOR_COLOR_BACK_MIRROR_FILL = IM_COL32(150, 120, 200, 22);
			const ImU32 EDITOR_COLOR_BACK_MIRROR = IM_COL32(190, 150, 255, 255);
			const ImU32 EDITOR_COLOR_LIGHT = IM_COL32(255, 240, 120, 220);
			const ImU32 EDITOR_COLOR_CHARACTER = IM_COL32(110, 230, 120, 255);
			const ImU32 EDITOR_COLOR_PLAYER_START = IM_COL32(110, 230, 120, 255);
			const ImU32 EDITOR_COLOR_ERROR = IM_COL32(255, 120, 100, 255);
			const ImU32 EDITOR_COLOR_LABEL = IM_COL32(255, 255, 255, 170);


			/** 名前の入力欄（変更されたら true） */
			bool EditName(std::string& name)
			{
				char buffer[64];
				std::snprintf(buffer, sizeof(buffer), "%s", name.c_str());
				if (ImGui::InputText("Name", buffer, sizeof(buffer)))
				{
					name = buffer;
					return true;
				}
				return false;
			}


			/** 点と線分の距離 */
			float DistanceToSegment(const math::Vec2& p, const math::Vec2& a, const math::Vec2& b)
			{
				const math::Vec2 ab = b - a;
				const float lengthSq = ab.LengthSq();
				float t = lengthSq > 0.0f ? math::Dot(p - a, ab) / lengthSq : 0.0f;
				t = math::Clamp01(t);
				return (p - (a + ab * t)).Length();
			}
		}

		/************************************/
		// 公開関数


		void StageEditor::Draw(Stage& stage)
		{
			if (!isOpen)
			{
				return;
			}

			if (!IsSelectionValid(stage))
			{
				m_selection = Selection();
			}

			UpdateHistory(stage);

			ImGui::SetNextWindowSize(ImVec2(1200.0f, 680.0f), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Stage Editor", &isOpen))
			{
				HandleShortcuts(stage);
				DrawToolbar(stage);

				ImGui::BeginChild("##StageEditorLeft", ImVec2(LEFT_PANE_WIDTH, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
				DrawObjectList(stage);
				ImGui::Separator();
				DrawProperties(stage);
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("##StageEditorCanvas", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				DrawCanvas(stage);
				ImGui::EndChild();
			}
			ImGui::End();
		}


		void StageEditor::AddCharacter(const char* label, const actor::CharacterBase* character)
		{
			m_characters.emplace_back(label, character);
		}


		/************************************/
		// 上部のツールバー


		void StageEditor::DrawToolbar(Stage& stage)
		{
			// 保存・読み込み（ファイルは StageCatalog.h の今のステージのもの）
			ImGui::BeginDisabled(m_stageInfo == nullptr);
			if (ImGui::Button("Save JSON"))
			{
				m_fileMessages.clear();
				m_isFileSucceeded = SaveStageToJson(stage, *m_stageInfo, m_fileMessages);
				if (m_isFileSucceeded) { m_fileMessages.insert(m_fileMessages.begin(), std::string("Saved: ") + m_stageInfo->objectJsonPath + (m_stageInfo->gimmickJsonPath != nullptr ? std::string(" / ") + m_stageInfo->gimmickJsonPath : std::string())); }
			}
			if (m_stageInfo != nullptr)
			{
				ImGui::SetItemTooltip("%s\n%s", m_stageInfo->objectJsonPath, m_stageInfo->gimmickJsonPath != nullptr ? m_stageInfo->gimmickJsonPath : "(no gimmick file)");
			}
			ImGui::SameLine();
			if (ImGui::Button("Load JSON"))
			{
				m_fileMessages.clear();
				m_isFileSucceeded = LoadStageFromJson(stage, *m_stageInfo, m_fileMessages);
				if (m_isFileSucceeded) { m_fileMessages.insert(m_fileMessages.begin(), std::string("Loaded: ") + m_stageInfo->name); }
				m_selection = Selection();
			}
			ImGui::SetItemTooltip("Reload the stage from the JSON files. Unsaved changes are lost (Undo can bring them back).");
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Reset Runtime"))
			{
				stage.ResetRuntimeState();
			}
			ImGui::SameLine();
			ImGui::Checkbox("Snap", &m_isSnap);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80.0f);
			ImGui::DragFloat("Grid", &m_gridSize, 1.0f, 5.0f, 500.0f, "%.0f");
			ImGui::SameLine();
			ImGui::Checkbox("Light", &m_isShowLight);
			ImGui::SameLine();
			ImGui::Checkbox("Back Mirror", &m_isShowBackMirror);
			ImGui::SameLine();
			ImGui::BeginDisabled(m_undoStack.empty());
			if (ImGui::Button("Undo")) { Undo(stage); }
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(m_redoStack.empty());
			if (ImGui::Button("Redo")) { Redo(stage); }
			ImGui::EndDisabled();
			if (ImGui::Button("Fit View"))
			{
				// 全ての地形が見えるように（縦に長いステージでも収まるよう、幅と高さの両方で合わせる）
				const auto blocks = stage.GetGimmicks().View<StageBlock>();
				if (!blocks.empty())
				{
					math::Rect bounds = blocks.begin()->rect;
					for (const StageBlock& block : blocks)
					{
						bounds.min.x = math::Min(bounds.min.x, block.rect.min.x);
						bounds.min.y = math::Min(bounds.min.y, block.rect.min.y);
						bounds.max.x = math::Max(bounds.max.x, block.rect.max.x);
						bounds.max.y = math::Max(bounds.max.y, block.rect.max.y);
					}
					m_viewCenter = (bounds.min + bounds.max) * 0.5f;
					const float canvasWidth = math::Max(200.0f, m_canvasCenter.x * 2.0f - LEFT_PANE_WIDTH);
					const float canvasHeight = math::Max(200.0f, ImGui::GetContentRegionAvail().y);
					m_zoom = math::Clamp(math::Min(canvasWidth / (bounds.Width() * 1.2f), canvasHeight / (bounds.Height() * 1.2f)), MIN_ZOOM, MAX_ZOOM);
				}
			}

			// 追加ボタン（GimmickRegistry.cpp の一覧から作る）
			ImGui::TextUnformatted("Add:");
			for (const GimmickRegistryEntry& entry : GetGimmickRegistry())
			{
				ImGui::SameLine();
				if (ImGui::SmallButton(entry.buttonLabel)) { AddGimmick(stage, entry); }
			}
			ImGui::SameLine(); ImGui::TextDisabled("|");
			ImGui::SameLine();
			ImGui::BeginDisabled(m_selection.type != SelectionType::Gimmick);
			if (ImGui::SmallButton("Duplicate")) { DuplicateSelected(stage); }
			ImGui::SameLine();
			if (ImGui::SmallButton("Delete")) { DeleteSelected(stage); }
			ImGui::EndDisabled();

			DrawFileMessages();
		}


		void StageEditor::DrawFileMessages()
		{
			if (m_fileMessages.empty())
			{
				return;
			}
			// 1行目は結果、2行目からは気づいたこと（知らない type・型の違う値など）
			const ImVec4 color = ImGui::ColorConvertU32ToFloat4(m_isFileSucceeded ? EDITOR_COLOR_PLAYER_START : EDITOR_COLOR_ERROR);
			for (const std::string& message : m_fileMessages)
			{
				ImGui::TextColored(color, "%s", message.c_str());
			}
			if (ImGui::SmallButton("Clear messages"))
			{
				m_fileMessages.clear();
			}
		}


		/************************************/
		// 物の一覧


		void StageEditor::DrawObjectList(Stage& stage)
		{
			ImGui::TextUnformatted(stage.GetName().empty() ? "(no name)" : stage.GetName().c_str());

			if (ImGui::Selectable("Back Mirror", m_selection.type == SelectionType::BackMirror))
			{
				m_selection = { SelectionType::BackMirror, 0 };
			}

			// 種類ごと（GimmickRegistry.cpp の並び順）に、その種類のギミックを並べる
			const GimmickManager& gimmicks = stage.GetGimmicks();
			for (const GimmickRegistryEntry& entry : GetGimmickRegistry())
			{
				int count = 0;
				for (const std::unique_ptr<GimmickBase>& gimmick : gimmicks.GetAll())
				{
					if (std::strcmp(gimmick->GetTypeName(), entry.typeName) == 0) { ++count; }
				}

				char header[96];
				std::snprintf(header, sizeof(header), "%s (%d)###list_%s", entry.typeName, count, entry.typeName);
				if (!ImGui::TreeNodeEx(header, ImGuiTreeNodeFlags_DefaultOpen))
				{
					continue;
				}
				for (size_t i = 0; i < gimmicks.GetCount(); ++i)
				{
					const GimmickBase& gimmick = *gimmicks.Get(i);
					if (std::strcmp(gimmick.GetTypeName(), entry.typeName) != 0)
					{
						continue;
					}
					ImGui::PushID(static_cast<int>(i));
					const bool isSelected = m_selection.type == SelectionType::Gimmick && m_selection.index == static_cast<int>(i);
					if (ImGui::Selectable(gimmick.name.empty() ? "(no name)" : gimmick.name.c_str(), isSelected))
					{
						m_selection = { SelectionType::Gimmick, static_cast<int>(i) };
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
		}


		/************************************/
		// 選択中の物の数値


		void StageEditor::DrawProperties(Stage& stage)
		{
			ImGui::PushID("Properties");

			switch (m_selection.type)
			{
			case SelectionType::None:
			{
				// 何も選んでいないときは、ステージの設定
				ImGui::TextDisabled("Stage settings (select an object to edit it)");
				std::string name = stage.GetName();
				if (EditName(name)) { stage.SetName(name); }
				math::Vec2 playerStart = stage.GetPlayerStart();
				if (ImGui::DragFloat2("Player Start", &playerStart.x, 1.0f, 0.0f, 0.0f, "%.1f"))
				{
					stage.SetPlayerStart(playerStart);
				}
				ImGui::SetItemTooltip("Feet position of the player at the start. Used after Save JSON and Reload.");
				break;
			}

			case SelectionType::BackMirror:
			{
				BackMirror& backMirror = stage.GetBackMirror();
				ImGui::Checkbox("Enabled", &backMirror.isEnabled);
				ImGui::DragFloat("Plane Z", &backMirror.planeZ, 1.0f, -5000.0f, 5000.0f, "%.1f");
				ImGui::SetItemTooltip("Depth of the mirror surface. Match it to the back edge of the floor.");
				m_inspector.EditRect(backMirror.frame);
				ImGui::TextWrapped("The mirror on the back wall. It reflects the room and the player (visual only).");
				break;
			}

			case SelectionType::Gimmick:
			{
				GimmickBase* gimmick = GetSelectedGimmick(stage);
				if (gimmick == nullptr)
				{
					break;
				}
				ImGui::TextDisabled("%s", gimmick->GetTypeName());

				// 名前（信号を出す物の名前を変えたら、扉や霧のつながりも新しい名前に付け替える）
				const std::string oldName = gimmick->name;
				if (EditName(gimmick->name) && gimmick->IsSignalSource())
				{
					stage.GetGimmicks().RenameSignal(oldName, gimmick->name);
				}

				// そのギミック自身の欄（信号の選択肢は毎回集め直す）
				stage.GetGimmicks().CollectSignalNames(m_inspector.signalNames);
				gimmick->DrawInspector(m_inspector);
				break;
			}
			}

			ImGui::PopID();
		}


		/************************************/
		// 見取り図


		void StageEditor::DrawCanvas(Stage& stage)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
			ImVec2 canvasSize = ImGui::GetContentRegionAvail();
			if (canvasSize.x < 50.0f) { canvasSize.x = 50.0f; }
			if (canvasSize.y < 50.0f) { canvasSize.y = 50.0f; }
			const ImVec2 canvasMax(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y);
			m_canvasCenter = ImVec2(canvasMin.x + canvasSize.x * 0.5f, canvasMin.y + canvasSize.y * 0.5f);

			// マウス入力を受ける透明なボタン
			ImGui::InvisibleButton("##canvas", canvasSize,
				ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
			const bool isHovered = ImGui::IsItemHovered();
			const bool isActive = ImGui::IsItemActive();
			const ImGuiIO& io = ImGui::GetIO();
			const math::Vec2 mouseWorld = ScreenToWorld(io.MousePos);
			const GizmoAxis hoveredAxis = (isHovered && !m_isDraggingObject) ? HitTestGizmo(stage, io.MousePos) : GizmoAxis::None;

			// ---- 入力 ----
			// ホイールで拡大縮小（マウスの位置を中心に）
			if (isHovered && io.MouseWheel != 0.0f)
			{
				const float newZoom = math::Clamp(m_zoom * std::pow(1.15f, io.MouseWheel), MIN_ZOOM, MAX_ZOOM);
				m_viewCenter = mouseWorld - (mouseWorld - m_viewCenter) * (m_zoom / newZoom);
				m_zoom = newZoom;
			}
			// 右/中ドラッグで画面を動かす
			if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle)))
			{
				m_viewCenter.x -= io.MouseDelta.x / m_zoom;
				m_viewCenter.y += io.MouseDelta.y / m_zoom;
			}
			// 左クリックで選択
			if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (hoveredAxis != GizmoAxis::None)
				{
					// 矢印をつかんだ：選択はそのまま、その軸だけ動かす
					m_dragAxis = hoveredAxis;
				}
				else
				{
					// 本体をつかんだ：今まで通り自由に動かす
					m_selection = HitTest(stage, mouseWorld);
					m_dragAxis = GizmoAxis::None;
				}
				m_isDraggingObject = m_selection.type != SelectionType::None;
				if (m_isDraggingObject)
				{
					m_dragStartAnchor = GetAnchor(stage, m_selection);
					m_dragAccumulated = math::Vec2::Zero();
				}
			}
			// 左ドラッグで動かす
			if (m_isDraggingObject)
			{
				if (isActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
				{
					m_dragAccumulated.x += io.MouseDelta.x / m_zoom;
					m_dragAccumulated.y -= io.MouseDelta.y / m_zoom;

					// つかんだ軸以外の移動を捨てる
					math::Vec2 delta = m_dragAccumulated;
					if (m_dragAxis == GizmoAxis::X) { delta.y = 0.0f; }
					if (m_dragAxis == GizmoAxis::Y) { delta.x = 0.0f; }

					math::Vec2 anchor = m_dragStartAnchor + delta;
					if (m_isSnap)
					{
						// 動かしている軸だけスナップする
						if (m_dragAxis != GizmoAxis::Y) { anchor.x = Snap(anchor.x); }
						if (m_dragAxis != GizmoAxis::X) { anchor.y = Snap(anchor.y); }
					}
					SetAnchor(stage, m_selection, anchor);
				}
				else
				{
					m_isDraggingObject = false;
					m_dragAxis = GizmoAxis::None;
				}
			}

			// ---- 描画 ----
			drawList->PushClipRect(canvasMin, canvasMax, true);
			drawList->AddRectFilled(canvasMin, canvasMax, EDITOR_COLOR_BACKGROUND);

			// グリッド
			if (m_gridSize * m_zoom >= 6.0f)
			{
				const math::Vec2 worldMin = ScreenToWorld(ImVec2(canvasMin.x, canvasMax.y));
				const math::Vec2 worldMax = ScreenToWorld(ImVec2(canvasMax.x, canvasMin.y));
				for (float x = std::floor(worldMin.x / m_gridSize) * m_gridSize; x <= worldMax.x; x += m_gridSize)
				{
					const float sx = WorldToScreen({ x, 0.0f }).x;
					drawList->AddLine(ImVec2(sx, canvasMin.y), ImVec2(sx, canvasMax.y), x == 0.0f ? EDITOR_COLOR_AXIS : EDITOR_COLOR_GRID);
				}
				for (float y = std::floor(worldMin.y / m_gridSize) * m_gridSize; y <= worldMax.y; y += m_gridSize)
				{
					const float sy = WorldToScreen({ 0.0f, y }).y;
					drawList->AddLine(ImVec2(canvasMin.x, sy), ImVec2(canvasMax.x, sy), y == 0.0f ? EDITOR_COLOR_AXIS : EDITOR_COLOR_GRID);
				}
			}

			// 矩形を描く
			auto drawRect = [&](const math::Rect& rect, const ImU32 fillColor, const ImU32 outlineColor)
				{
					const ImVec2 a = WorldToScreen({ rect.min.x, rect.max.y });
					const ImVec2 b = WorldToScreen({ rect.max.x, rect.min.y });
					if (fillColor != 0) { drawList->AddRectFilled(a, b, fillColor); }
					if (outlineColor != 0) { drawList->AddRect(a, b, outlineColor, 0.0f, 0, 2.0f); }
				};
			auto drawLabel = [&](const math::Vec2& world, const std::string& text)
				{
					const ImVec2 p = WorldToScreen(world);
					drawList->AddText(ImVec2(p.x + 3.0f, p.y + 2.0f), EDITOR_COLOR_LABEL, text.c_str());
				};

			// 奥の鏡（部屋の奥の壁。いちばん後ろに描く）
			const BackMirror& backMirror = stage.GetBackMirror();
			if (backMirror.isEnabled && m_isShowBackMirror)
			{
				drawRect(backMirror.frame, EDITOR_COLOR_BACK_MIRROR_FILL, 0);
			}

			// ギミック（後ろの層から順に。同じ層の中では置いた順）
			const EditorCanvas canvas(drawList, m_viewCenter, m_zoom, m_canvasCenter);
			const GimmickManager& gimmicks = stage.GetGimmicks();
			auto drawLayer = [&](const EditorLayer layer)
				{
					for (size_t i = 0; i < gimmicks.GetCount(); ++i)
					{
						const GimmickBase& gimmick = *gimmicks.Get(i);
						if (gimmick.GetEditorLayer() != layer)
						{
							continue;
						}
						const bool isSelected = m_selection.type == SelectionType::Gimmick && m_selection.index == static_cast<int>(i);
						gimmick.DrawEditor(canvas, isSelected);
					}
				};
			drawLayer(EditorLayer::Back);
			drawLayer(EditorLayer::Middle);

			// 光
			if (m_isShowLight)
			{
				for (const LightSegment& segment : stage.GetLightResult().segments)
				{
					drawList->AddLine(WorldToScreen(segment.start), WorldToScreen(segment.end), EDITOR_COLOR_LIGHT, 2.0f);
				}
			}

			// 光より手前に描く物（反射鏡・光源）
			drawLayer(EditorLayer::Front);

			// 奥の鏡の枠（枠線をクリックすると選択できる）
			if (backMirror.isEnabled && m_isShowBackMirror)
			{
				const bool isSelectedFrame = m_selection.type == SelectionType::BackMirror;
				drawRect(backMirror.frame, 0, EditorCanvas::Pick(isSelectedFrame, EDITOR_COLOR_BACK_MIRROR));
				drawLabel({ backMirror.frame.min.x, backMirror.frame.max.y }, "Back Mirror");
			}

			// プレイヤーの開始位置
			drawList->AddCircleFilled(WorldToScreen(stage.GetPlayerStart()), 5.0f, EDITOR_COLOR_PLAYER_START);
			drawLabel(stage.GetPlayerStart(), "Start");

			// キャラクター
			for (const auto& character : m_characters)
			{
				if (character.second == nullptr) { continue; }
				const math::Rect bounds = character.second->GetController().GetBounds();
				drawRect(bounds, 0, EDITOR_COLOR_CHARACTER);
				const ImVec2 head = WorldToScreen({ bounds.min.x, bounds.max.y });
				drawList->AddText(ImVec2(head.x, head.y - 16.0f), EDITOR_COLOR_CHARACTER, character.first.c_str());
			}

			// 選択中の物の移動矢印（いちばん上に描く）
			if (m_selection.type != SelectionType::None)
			{
				DrawGizmo(drawList, stage, hoveredAxis);
			}

			// マウス位置の座標
			if (isHovered)
			{
				char text[64];
				std::snprintf(text, sizeof(text), "(%.0f, %.0f)  zoom %.2f", mouseWorld.x, mouseWorld.y, m_zoom);
				drawList->AddText(ImVec2(canvasMin.x + 6.0f, canvasMax.y - 20.0f), EDITOR_COLOR_LABEL, text);
			}

			drawList->PopClipRect();
		}

		void StageEditor::DrawGizmo(ImDrawList* drawList, const Stage& stage, const GizmoAxis hovered) const
		{
			const ImVec2 o = WorldToScreen(GetAnchor(stage, m_selection));

			auto drawArrow = [&](const ImVec2& dir, const ImU32 baseColor, const GizmoAxis axis)
				{
					// つかんでいる軸・マウスが乗っている軸を黄色にする
					const bool isHot = m_isDraggingObject ? (m_dragAxis == axis) : (hovered == axis);
					const ImU32 color = isHot ? EDITOR_COLOR_GIZMO_HOT : baseColor;

					const ImVec2 end(o.x + dir.x * GIZMO_LENGTH_PX, o.y + dir.y * GIZMO_LENGTH_PX);
					const ImVec2 tip(end.x + dir.x * GIZMO_HEAD_PX, end.y + dir.y * GIZMO_HEAD_PX);
					const ImVec2 side(-dir.y * GIZMO_HEAD_PX * 0.5f, dir.x * GIZMO_HEAD_PX * 0.5f);
					drawList->AddLine(o, end, color, 3.0f);
					drawList->AddTriangleFilled(tip, ImVec2(end.x + side.x, end.y + side.y), ImVec2(end.x - side.x, end.y - side.y), color);
				};

			drawArrow(ImVec2(1.0f, 0.0f), EDITOR_COLOR_GIZMO_X, GizmoAxis::X);
			drawArrow(ImVec2(0.0f, -1.0f), EDITOR_COLOR_GIZMO_Y, GizmoAxis::Y);
			drawList->AddCircleFilled(o, 3.0f, EDITOR_COLOR_LABEL);
		}

		/************************************/
		// 編集操作


		void StageEditor::AddGimmick(Stage& stage, const GimmickRegistryEntry& entry)
		{
			const math::Vec2 position(Snap(m_viewCenter.x), Snap(m_viewCenter.y));
			std::unique_ptr<GimmickBase> gimmick = entry.create(position);
			gimmick->name = stage.GetGimmicks().MakeUniqueName(entry.namePrefix);
			stage.GetGimmicks().Add(std::move(gimmick));
			m_selection = { SelectionType::Gimmick, static_cast<int>(stage.GetGimmicks().GetCount()) - 1 };
		}


		void StageEditor::DuplicateSelected(Stage& stage)
		{
			const GimmickBase* source = GetSelectedGimmick(stage);
			if (source == nullptr)
			{
				return;
			}
			// 名前の頭は追加ボタンと同じにする（"Door" → "Door3" など）
			const GimmickRegistryEntry* entry = FindGimmickRegistryEntry(source->GetTypeName());
			std::unique_ptr<GimmickBase> copy = source->Clone();
			copy->name = stage.GetGimmicks().MakeUniqueName(entry != nullptr ? entry->namePrefix : source->GetTypeName());
			copy->SetAnchor(source->GetAnchor() + math::Vec2(DUPLICATE_OFFSET, 0.0f));
			stage.GetGimmicks().Add(std::move(copy));
			m_selection.index = static_cast<int>(stage.GetGimmicks().GetCount()) - 1;
		}


		void StageEditor::DeleteSelected(Stage& stage)
		{
			const GimmickBase* gimmick = GetSelectedGimmick(stage);
			if (gimmick == nullptr)
			{
				return;
			}
			const std::string name = gimmick->name;
			const bool isSignalSource = gimmick->IsSignalSource();
			stage.GetGimmicks().Remove(static_cast<size_t>(m_selection.index));
			// 信号を出す物を消したら、扉や霧のつながりからも外す
			if (isSignalSource)
			{
				stage.GetGimmicks().RemoveSignal(name);
			}
			m_selection = Selection();
			m_isDraggingObject = false;
		}


		bool StageEditor::IsSelectionValid(const Stage& stage) const
		{
			switch (m_selection.type)
			{
			case SelectionType::None:		return true;
			case SelectionType::BackMirror:	return true;
			case SelectionType::Gimmick:	return m_selection.index >= 0 && m_selection.index < static_cast<int>(stage.GetGimmicks().GetCount());
			}
			return false;
		}


		StageEditor::Selection StageEditor::HitTest(const Stage& stage, const math::Vec2& worldPos) const
		{
			const float pickDistance = PICK_DISTANCE_PX / m_zoom;
			const GimmickManager& gimmicks = stage.GetGimmicks();

			// 手前に描いた物から順に調べる（同じ層の中では、後から置いた＝上に描いた物を優先）
			auto hitLayer = [&](const EditorLayer layer)
				{
					for (size_t i = gimmicks.GetCount(); i > 0; --i)
					{
						const GimmickBase& gimmick = *gimmicks.Get(i - 1);
						if (gimmick.GetEditorLayer() == layer && gimmick.HitTest(worldPos, pickDistance))
						{
							return Selection{ SelectionType::Gimmick, static_cast<int>(i - 1) };
						}
					}
					return Selection();
				};

			Selection hit = hitLayer(EditorLayer::Front);
			if (hit.type != SelectionType::None) { return hit; }
			hit = hitLayer(EditorLayer::Middle);
			if (hit.type != SelectionType::None) { return hit; }

			// 奥の鏡は部屋全体を覆うので、枠線の近くをクリックしたときだけ選ぶ
			const BackMirror& backMirror = stage.GetBackMirror();
			if (backMirror.isEnabled && m_isShowBackMirror)
			{
				const math::Rect& frame = backMirror.frame;
				const bool isInsideOuter = worldPos.x >= frame.min.x - pickDistance && worldPos.x <= frame.max.x + pickDistance
					&& worldPos.y >= frame.min.y - pickDistance && worldPos.y <= frame.max.y + pickDistance;
				const bool isInsideInner = worldPos.x > frame.min.x + pickDistance && worldPos.x < frame.max.x - pickDistance
					&& worldPos.y > frame.min.y + pickDistance && worldPos.y < frame.max.y - pickDistance;
				if (isInsideOuter && !isInsideInner)
				{
					return { SelectionType::BackMirror, 0 };
				}
			}

			// いちばん後ろ（地形ブロック）
			return hitLayer(EditorLayer::Back);
		}


		StageEditor::GizmoAxis StageEditor::HitTestGizmo(const Stage& stage, const ImVec2& mousePos) const
		{
			if (m_selection.type == SelectionType::None)
			{
				return GizmoAxis::None;
			}
			const ImVec2 o = WorldToScreen(GetAnchor(stage, m_selection));
			const math::Vec2 mouse{ mousePos.x, mousePos.y };
			const math::Vec2 origin{ o.x, o.y };
			const math::Vec2 xEnd{ o.x + GIZMO_LENGTH_PX + GIZMO_HEAD_PX, o.y };
			const math::Vec2 yEnd{ o.x, o.y - GIZMO_LENGTH_PX - GIZMO_HEAD_PX };	// 画面は下が+Yなので、上向きはマイナス

			if (DistanceToSegment(mouse, origin, xEnd) <= PICK_DISTANCE_PX) { return GizmoAxis::X; }
			if (DistanceToSegment(mouse, origin, yEnd) <= PICK_DISTANCE_PX) { return GizmoAxis::Y; }
			return GizmoAxis::None;
		}

		math::Vec2 StageEditor::GetAnchor(const Stage& stage, const Selection& selection) const
		{
			switch (selection.type)
			{
			case SelectionType::BackMirror:
				return stage.GetBackMirror().frame.min;
			case SelectionType::Gimmick:
			{
				const GimmickBase* gimmick = stage.GetGimmicks().Get(static_cast<size_t>(selection.index));
				return gimmick != nullptr ? gimmick->GetAnchor() : math::Vec2::Zero();
			}
			default:
				return math::Vec2::Zero();
			}
		}


		void StageEditor::SetAnchor(Stage& stage, const Selection& selection, const math::Vec2& anchor)
		{
			switch (selection.type)
			{
			case SelectionType::BackMirror:
			{
				// 大きさを保ったまま左下を動かす
				math::Rect& frame = stage.GetBackMirror().frame;
				const math::Vec2 size(frame.Width(), frame.Height());
				frame.min = anchor;
				frame.max = anchor + size;
				break;
			}
			case SelectionType::Gimmick:
				if (GimmickBase* gimmick = stage.GetGimmicks().Get(static_cast<size_t>(selection.index)))
				{
					gimmick->SetAnchor(anchor);
				}
				break;
			default:
				break;
			}
		}


		GimmickBase* StageEditor::GetSelectedGimmick(Stage& stage) const
		{
			if (m_selection.type != SelectionType::Gimmick)
			{
				return nullptr;
			}
			return stage.GetGimmicks().Get(static_cast<size_t>(m_selection.index));
		}


		StageEditor::Snapshot StageEditor::CaptureSnapshot(const Stage& stage) const
		{
			Snapshot s;
			s.name = stage.GetName();
			s.playerStart = stage.GetPlayerStart();
			s.backMirror = stage.GetBackMirror();
			s.gimmicks = stage.GetGimmicks();
			return s;
		}


		void StageEditor::RestoreSnapshot(Stage& stage, const Snapshot& s)
		{
			stage.SetName(s.name.c_str());
			stage.SetPlayerStart(s.playerStart);
			stage.GetBackMirror() = s.backMirror;
			stage.GetGimmicks() = s.gimmicks;

			// 消えた物を選んでいたら選択を外す
			if (!IsSelectionValid(stage))
			{
				m_selection = Selection();
			}
			m_isDraggingObject = false;
			m_dragAxis = GizmoAxis::None;
		}


		void StageEditor::UpdateHistory(const Stage& stage)
		{
			// ドラッグ中・入力中は、操作が終わるまで待つ（1回のドラッグを1回の履歴にする）
			if (ImGui::IsAnyItemActive() || m_isDraggingObject)
			{
				return;
			}

			const std::string code = StageToJsonText(stage);
			if (!m_isHistoryReady)
			{
				// 最初のフレーム：今の状態を基準にする
				m_stableSnapshot = CaptureSnapshot(stage);
				m_stableCode = code;
				m_isHistoryReady = true;
				return;
			}
			if (code == m_stableCode)
			{
				return;
			}

			// 変わっていたら、変わる前の状態を Undo に積む
			m_undoStack.push_back(m_stableSnapshot);
			if (m_undoStack.size() > MAX_HISTORY)
			{
				m_undoStack.erase(m_undoStack.begin());
			}
			m_redoStack.clear();	// 新しい操作をしたら、やり直しの履歴は捨てる

			m_stableSnapshot = CaptureSnapshot(stage);
			m_stableCode = code;
		}


		void StageEditor::Undo(Stage& stage)
		{
			if (m_undoStack.empty())
			{
				return;
			}
			m_redoStack.push_back(CaptureSnapshot(stage));
			RestoreSnapshot(stage, m_undoStack.back());
			m_undoStack.pop_back();

			// 戻した状態を新しい基準にする（これを「変更」と判定しないため）
			m_stableSnapshot = CaptureSnapshot(stage);
			m_stableCode = StageToJsonText(stage);
		}


		void StageEditor::Redo(Stage& stage)
		{
			if (m_redoStack.empty())
			{
				return;
			}
			m_undoStack.push_back(CaptureSnapshot(stage));
			RestoreSnapshot(stage, m_redoStack.back());
			m_redoStack.pop_back();

			m_stableSnapshot = CaptureSnapshot(stage);
			m_stableCode = StageToJsonText(stage);
		}


		void StageEditor::HandleShortcuts(Stage& stage)
		{
			// エディタのウィンドウを触っているときだけ（ゲーム側のキー入力とぶつけないため）
			if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
			{
				return;
			}
			// 文字入力中（Name 欄など）は、入力欄自身の Undo に任せる
			const ImGuiIO& io = ImGui::GetIO();
			if (io.WantTextInput || ImGui::IsAnyItemActive())
			{
				return;
			}

			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
			{
				if (io.KeyShift) { Redo(stage); }	// Ctrl+Shift+Z でもやり直せるように
				else { Undo(stage); }
			}
			else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
			{
				Redo(stage);
			}
		}


		float StageEditor::Snap(const float value) const
		{
			if (!m_isSnap || m_gridSize <= 0.0f)
			{
				return value;
			}
			return std::round(value / m_gridSize) * m_gridSize;
		}


		ImVec2 StageEditor::WorldToScreen(const math::Vec2& world) const
		{
			// 画面は下向きが +Y なので、Y を反転する
			return ImVec2(
				m_canvasCenter.x + (world.x - m_viewCenter.x) * m_zoom,
				m_canvasCenter.y - (world.y - m_viewCenter.y) * m_zoom);
		}


		math::Vec2 StageEditor::ScreenToWorld(const ImVec2& screen) const
		{
			return {
				m_viewCenter.x + (screen.x - m_canvasCenter.x) / m_zoom,
				m_viewCenter.y - (screen.y - m_canvasCenter.y) / m_zoom
			};
		}


		StageEditor::StageEditor()
			: m_viewCenter(600.0f, 250.0f)
			, m_zoom(0.45f)
			, m_canvasCenter(0.0f, 0.0f)
			, m_isSnap(true)
			, m_gridSize(25.0f)
			, m_isShowLight(true)
			, m_isShowBackMirror(true)
			, m_isDraggingObject(false)
			, m_dragStartAnchor()
			, m_dragAccumulated()
			, m_dragAxis(GizmoAxis::None)
		{}
	}
}
