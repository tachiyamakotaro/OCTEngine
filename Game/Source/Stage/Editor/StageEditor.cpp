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
			/** 物の一覧の欄の幅 */
			constexpr float LEFT_PANE_WIDTH = 260.0f;
			/** 右の詳細パネルの幅（最初の幅・いちばん狭い幅） */
			constexpr float INSPECTOR_DEFAULT_WIDTH = 340.0f;
			constexpr float INSPECTOR_MIN_WIDTH = 220.0f;
			/** 見取り図のいちばん狭い幅（詳細パネルを広げすぎても、これ以上は狭くしない） */
			constexpr float CANVAS_MIN_WIDTH = 200.0f;
			/** 見取り図と詳細パネルの境目（つかめる幅） */
			constexpr float SPLITTER_WIDTH = 6.0f;
			/** 追加ボタンを何行に分けるか */
			constexpr size_t ADD_BUTTON_ROWS = 2;
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
			/** 形を変える取っ手の大きさ（四角の半分の幅。ピクセル） */
			constexpr float HANDLE_HALF_PX = 5.0f;
			/** 覚えておく履歴の数 */
			constexpr size_t MAX_HISTORY = 100;

			const ImU32 EDITOR_COLOR_GIZMO_X = IM_COL32(235, 70, 70, 255);
			const ImU32 EDITOR_COLOR_GIZMO_Y = IM_COL32(90, 220, 90, 255);
			const ImU32 EDITOR_COLOR_GIZMO_HOT = IM_COL32(255, 230, 60, 255);
			const ImU32 EDITOR_COLOR_HANDLE = IM_COL32(255, 255, 255, 230);
			const ImU32 EDITOR_COLOR_HANDLE_BORDER = IM_COL32(20, 20, 30, 255);

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
			/** 範囲選択の四角（中の塗りと枠） */
			const ImU32 EDITOR_COLOR_BOX_FILL = IM_COL32(80, 150, 230, 60);
			const ImU32 EDITOR_COLOR_BOX_LINE = IM_COL32(60, 140, 255, 220);


			/** 2点を角にした四角（どちら向きにドラッグしても min < max になるように） */
			math::Rect MakeRect(const math::Vec2& a, const math::Vec2& b)
			{
				math::Rect rect;
				rect.min = math::Vec2(math::Min(a.x, b.x), math::Min(a.y, b.y));
				rect.max = math::Vec2(math::Max(a.x, b.x), math::Max(a.y, b.y));
				return rect;
			}


			/**
			 * @brief 範囲選択の四角に少しでもかかっているか
			 * @note 縁にぴったり触れているだけでも選ぶ（幅0の線のような物も選べるように、Rect::Overlaps ではなく「以下」で比べる）
			 */
			bool IsTouchingBox(const math::Rect& bounds, const math::Rect& box)
			{
				return bounds.min.x <= box.max.x && bounds.max.x >= box.min.x
					&& bounds.min.y <= box.max.y && bounds.max.y >= box.min.y;
			}


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

			ValidateSelection(stage);

			UpdateHistory(stage);

			ImGui::SetNextWindowSize(ImVec2(1200.0f, 680.0f), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Stage Editor", &isOpen))
			{
				HandleShortcuts(stage);
				DrawToolbar(stage);

				// 左：物の一覧（右の端をドラッグすると幅を変えられる）
				ImGui::BeginChild("##StageEditorLeft", ImVec2(LEFT_PANE_WIDTH, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
				DrawObjectList(stage);
				ImGui::EndChild();

				ImGui::SameLine();

				// 残りの幅を「見取り図 | 境目 | 詳細パネル」に分ける（ウィンドウが狭いときは詳細パネルを縮める）
				const ImVec2 available = ImGui::GetContentRegionAvail();
				const float maxInspectorWidth = math::Max(INSPECTOR_MIN_WIDTH, available.x - SPLITTER_WIDTH - CANVAS_MIN_WIDTH);
				m_inspectorWidth = math::Clamp(m_inspectorWidth, INSPECTOR_MIN_WIDTH, maxInspectorWidth);
				const float canvasWidth = math::Max(CANVAS_MIN_WIDTH, available.x - SPLITTER_WIDTH - m_inspectorWidth);

				// 真ん中：見取り図
				ImGui::BeginChild("##StageEditorCanvas", ImVec2(canvasWidth, 0.0f), ImGuiChildFlags_Borders,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				DrawCanvas(stage);
				ImGui::EndChild();

				// 境目（ドラッグで詳細パネルの幅を変える）
				ImGui::SameLine(0.0f, 0.0f);
				DrawInspectorSplitter(available.y);
				ImGui::SameLine(0.0f, 0.0f);

				// 右：詳細パネル
				ImGui::BeginChild("##StageEditorInspector", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
				DrawInspectorPanel(stage);
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
				SelectNone();
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
					// 見取り図の大きさ（詳細パネルのぶんを引いた、実際に描いている大きさ）
					const float canvasWidth = math::Max(200.0f, m_canvasSize.x);
					const float canvasHeight = math::Max(200.0f, m_canvasSize.y);
					m_zoom = math::Clamp(math::Min(canvasWidth / (bounds.Width() * 1.2f), canvasHeight / (bounds.Height() * 1.2f)), MIN_ZOOM, MAX_ZOOM);
				}
			}

			// 追加ボタン（複製・削除・回転のボタンは右の詳細パネルに移した）
			DrawAddButtons(stage);

			DrawFileMessages();
		}


		void StageEditor::DrawAddButtons(Stage& stage)
		{
			// GimmickRegistry.cpp の一覧を ADD_BUTTON_ROWS 行に分けて並べる（1行だと右のほうがウィンドウからはみ出すため）
			// ウィンドウが狭くて1行に入りきらないときは、はみ出す前に折り返す
			const std::vector<GimmickRegistryEntry>& registry = GetGimmickRegistry();
			if (registry.empty())
			{
				return;
			}
			const size_t buttonsPerRow = (registry.size() + ADD_BUTTON_ROWS - 1) / ADD_BUTTON_ROWS;
			const ImGuiStyle& style = ImGui::GetStyle();

			ImGui::TextUnformatted("Add:");
			ImGui::SameLine();
			// 2行目からも「Add:」の右にそろえる
			const float rowStartX = ImGui::GetCursorPosX();
			const float rightEdge = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

			for (size_t i = 0; i < registry.size(); ++i)
			{
				const GimmickRegistryEntry& entry = registry[i];
				if (i > 0)
				{
					const float buttonWidth = ImGui::CalcTextSize(entry.buttonLabel, nullptr, true).x + style.FramePadding.x * 2.0f;
					const bool isRowEnd = (i % buttonsPerRow) == 0;
					const bool isFit = ImGui::GetItemRectMax().x + style.ItemSpacing.x + buttonWidth <= rightEdge;
					if (!isRowEnd && isFit)
					{
						ImGui::SameLine();
					}
					else
					{
						// 次の行へ
						ImGui::SetCursorPosX(rowStartX);
					}
				}
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::SmallButton(entry.buttonLabel)) { AddGimmick(stage, entry); }
				ImGui::PopID();
			}
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
				SelectSingle({ SelectionType::BackMirror, 0 });
			}

			// 種類ごと（GimmickRegistry.cpp の並び順）に、その種類のギミックを並べる
			const GimmickManager& gimmicks = stage.GetGimmicks();
			const std::vector<GimmickRegistryEntry>& registry = GetGimmickRegistry();
			for (size_t entryIndex = 0; entryIndex < registry.size(); ++entryIndex)
			{
				const GimmickRegistryEntry& entry = registry[entryIndex];
				// 同じクラスを初期値違いで2つ登録しているとき（Ice (Melt) と Ice (Keep) など）は、一覧には1回だけ出す
				// （2回出すと、同じ ID の項目が2つになって ImGui が警告を出す）
				bool isAlreadyListed = false;
				for (size_t previous = 0; previous < entryIndex; ++previous)
				{
					if (std::strcmp(registry[previous].typeName, entry.typeName) == 0)
					{
						isAlreadyListed = true;
						break;
					}
				}
				if (isAlreadyListed)
				{
					continue;
				}

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
					const int index = static_cast<int>(i);
					if (ImGui::Selectable(gimmick.name.empty() ? "(no name)" : gimmick.name.c_str(), IsGimmickSelected(index)))
					{
						// Ctrl+クリックで選択に足す・外す。ふつうのクリックはその物だけを選ぶ
						if (ImGui::GetIO().KeyCtrl) { ToggleGimmickSelection(index); }
						else { SelectSingle({ SelectionType::Gimmick, index }); }
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
		}


		/************************************/
		// 右の詳細パネル


		void StageEditor::DrawInspectorPanel(Stage& stage)
		{
			ImGui::PushID("Inspector");
			ImGui::SeparatorText("Inspector");

			// ---- 見出し（何を選んでいるか） ----
			switch (m_selection.type)
			{
			case SelectionType::None:
				ImGui::TextUnformatted("Stage");
				ImGui::SameLine();
				ImGui::TextDisabled("(nothing selected)");
				ImGui::TextDisabled("Drag on empty space to select an area.");
				break;
			case SelectionType::BackMirror:
				ImGui::TextUnformatted("Back Mirror");
				break;
			case SelectionType::Gimmick:
				if (m_selectedGimmicks.size() > 1)
				{
					ImGui::Text("%d objects selected", static_cast<int>(m_selectedGimmicks.size()));
				}
				else if (const GimmickBase* gimmick = GetSelectedGimmick(stage))
				{
					ImGui::TextUnformatted(gimmick->name.empty() ? "(no name)" : gimmick->name.c_str());
					ImGui::SameLine();
					ImGui::TextDisabled("(%s)", gimmick->GetTypeName());
				}
				break;
			}

			// ---- 編集ボタン（ギミックを選んでいるときだけ。いくつか選んでいたら全部に効く） ----
			// ボタンを押すと選択が変わる（複製した物・選択なし）ので、ここから下はギミックのポインタを持ち越さない
			if (m_selection.type == SelectionType::Gimmick)
			{
				if (ImGui::SmallButton("Duplicate")) { DuplicateSelected(stage); }
				ImGui::SetItemTooltip("Duplicate all selected objects (Ctrl+D).");
				ImGui::SameLine();
				if (ImGui::SmallButton("Delete")) { DeleteSelected(stage); }
				ImGui::SetItemTooltip("Delete all selected objects (Delete key).");
				ImGui::SameLine();
				if (ImGui::SmallButton("Rotate L")) { RotateSelected(stage, 1); }
				ImGui::SetItemTooltip("Rotate 90 degrees counterclockwise (Shift+R).");
				ImGui::SameLine();
				if (ImGui::SmallButton("Rotate R")) { RotateSelected(stage, -1); }
				ImGui::SetItemTooltip("Rotate 90 degrees clockwise (R).\nBlocks: swap width and height around the center.\nLamps / mirrors: turn the angle.");
			}

			// ---- いくつか選んでいるとき：選んだ物の一覧（クリックでその物だけを選ぶ） ----
			if (m_selectedGimmicks.size() > 1)
			{
				if (ImGui::CollapsingHeader("Selected", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::TextDisabled("Click a name to edit only that object.");
					// 一覧のクリックで m_selectedGimmicks が変わるので、コピーを回す
					const std::vector<int> selected = m_selectedGimmicks;
					for (const int index : selected)
					{
						const GimmickBase* gimmick = stage.GetGimmicks().Get(static_cast<size_t>(index));
						if (gimmick == nullptr)
						{
							continue;
						}
						ImGui::PushID(index);
						char label[128];
						std::snprintf(label, sizeof(label), "%s  (%s)", gimmick->name.empty() ? "(no name)" : gimmick->name.c_str(), gimmick->GetTypeName());
						if (ImGui::Selectable(label, m_selection.index == index))
						{
							SelectSingle({ SelectionType::Gimmick, index });
						}
						ImGui::PopID();
					}
				}
				ImGui::PopID();
				return;
			}

			// ---- 数値（1つだけ選んでいるとき・何も選んでいないとき） ----
			if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
			{
				DrawProperties(stage);
			}

			ImGui::PopID();
		}


		void StageEditor::DrawInspectorSplitter(const float height)
		{
			// 見えないボタンを境目に置いて、ドラッグした分だけ詳細パネルの幅を変える（左へドラッグ → 広がる）
			ImGui::InvisibleButton("##InspectorSplitter", ImVec2(SPLITTER_WIDTH, math::Max(1.0f, height)));
			const bool isHovered = ImGui::IsItemHovered();
			const bool isActive = ImGui::IsItemActive();
			if (isHovered || isActive)
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
			if (isActive)
			{
				m_inspectorWidth -= ImGui::GetIO().MouseDelta.x;
			}

			// つかめる場所がわかるように、真ん中に線を描く
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			const float x = (min.x + max.x) * 0.5f;
			const ImU32 color = ImGui::GetColorU32(isActive ? ImGuiCol_SeparatorActive : (isHovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
			ImGui::GetWindowDrawList()->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), color, isActive || isHovered ? 2.0f : 1.0f);
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
				// 種類（クラス名）は詳細パネルの見出しに出している

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
			m_canvasSize = canvasSize;

			// マウス入力を受ける透明なボタン
			ImGui::InvisibleButton("##canvas", canvasSize,
				ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
			const bool isHovered = ImGui::IsItemHovered();
			const bool isActive = ImGui::IsItemActive();
			const ImGuiIO& io = ImGui::GetIO();
			const math::Vec2 mouseWorld = ScreenToWorld(io.MousePos);
			const GizmoAxis hoveredAxis = (isHovered && !m_isDraggingObject) ? HitTestGizmo(stage, io.MousePos) : GizmoAxis::None;
			const int hoveredHandle = (isHovered && !m_isDraggingObject && !m_isBoxSelecting) ? HitTestHandle(stage, io.MousePos) : -1;

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
			// 左クリック
			if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				// Shift / Ctrl を押しながら：今の選択に足す
				const bool isAdditive = io.KeyCtrl || io.KeyShift;
				m_isPendingSingleSelect = false;
				m_dragHandle = -1;
				if (hoveredHandle >= 0)
				{
					// 形を変える取っ手をつかんだ：選択はそのまま、取っ手だけ動かす（物は動かさない）
					m_dragHandle = hoveredHandle;
					m_dragAxis = GizmoAxis::None;
					m_dragHandleStart = GetHandleOwner(stage)->GetEditHandle(hoveredHandle);
					m_dragAccumulated = math::Vec2::Zero();
					m_isDraggingObject = true;
				}
				else if (hoveredAxis != GizmoAxis::None)
				{
					// 矢印をつかんだ：選択はそのまま、その軸だけ動かす（いくつか選んでいたら全部）
					m_dragAxis = hoveredAxis;
					BeginObjectDrag(stage);
				}
				else
				{
					m_dragAxis = GizmoAxis::None;
					const Selection hit = HitTest(stage, mouseWorld);
					if (hit.type == SelectionType::None)
					{
						// 何もないところ：範囲選択を始める
						m_isBoxSelecting = true;
						m_boxStart = mouseWorld;
						m_isBoxAdditive = isAdditive;
						if (!isAdditive)
						{
							SelectNone();
						}
					}
					else if (isAdditive && hit.type == SelectionType::Gimmick)
					{
						// Ctrl / Shift+クリック：その物を選択に足す・外す（動かさない）
						ToggleGimmickSelection(hit.index);
					}
					else if (hit.type == SelectionType::Gimmick && IsGimmickSelected(hit.index))
					{
						// 選択中の物をつかんだ：選択はそのまま全部動かす（つかんだ物をスナップの基準にする）
						// 動かさずに離したら、その物だけを選び直す（エクスプローラーと同じ）
						m_selection = hit;
						m_isPendingSingleSelect = m_selectedGimmicks.size() > 1;
						BeginObjectDrag(stage);
					}
					else
					{
						// 選んでいない物をつかんだ：その物だけを選んで動かす
						SelectSingle(hit);
						BeginObjectDrag(stage);
					}
				}
			}
			// 左ドラッグで動かす
			if (m_isDraggingObject)
			{
				if (isActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
				{
					m_dragAccumulated.x += io.MouseDelta.x / m_zoom;
					m_dragAccumulated.y -= io.MouseDelta.y / m_zoom;

					// 取っ手をつかんでいるとき：物は動かさず、取っ手を動かして形を変える
					GimmickBase* handleOwner = (m_dragHandle >= 0 && GetHandleOwner(stage) != nullptr) ? stage.GetGimmicks().Get(static_cast<size_t>(m_selection.index)) : nullptr;
					if (handleOwner != nullptr)
					{
						math::Vec2 position = m_dragHandleStart + m_dragAccumulated;
						if (m_isSnap)
						{
							position.x = Snap(position.x);
							position.y = Snap(position.y);
						}
						handleOwner->SetEditHandle(m_dragHandle, position);
					}
					else
					{
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
						MoveSelection(stage, anchor);
					}
				}
				else
				{
					// 選択中の物をクリックしただけ（ほとんど動かしていない）なら、その物だけを選び直す
					const float movedPixels = m_dragAccumulated.Length() * m_zoom;
					if (m_isPendingSingleSelect && movedPixels < io.MouseDragThreshold)
					{
						SelectSingle(m_selection);
					}
					m_isPendingSingleSelect = false;
					m_isDraggingObject = false;
					m_dragAxis = GizmoAxis::None;
					m_dragHandle = -1;
				}
			}
			// 範囲選択：離したときに、四角に少しでもかかった物を選ぶ
			if (m_isBoxSelecting && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				const ImVec2 startScreen = WorldToScreen(m_boxStart);
				const float dx = io.MousePos.x - startScreen.x;
				const float dy = io.MousePos.y - startScreen.y;
				// ほとんど動かしていない（ただのクリック）なら選ばない（クリックした時点で選択は外れている）
				if (dx * dx + dy * dy >= io.MouseDragThreshold * io.MouseDragThreshold)
				{
					SelectGimmicksInRect(stage, MakeRect(m_boxStart, mouseWorld), m_isBoxAdditive);
				}
				m_isBoxSelecting = false;
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
			// 範囲選択の途中は、離したら選ばれる物も選択の色で見せる
			const math::Rect boxRect = MakeRect(m_boxStart, mouseWorld);
			auto drawLayer = [&](const EditorLayer layer)
				{
					for (size_t i = 0; i < gimmicks.GetCount(); ++i)
					{
						const GimmickBase& gimmick = *gimmicks.Get(i);
						if (gimmick.GetEditorLayer() != layer)
						{
							continue;
						}
						const bool isInBox = m_isBoxSelecting && IsTouchingBox(gimmick.GetBounds(), boxRect);
						const bool isSelected = IsGimmickSelected(static_cast<int>(i)) || isInBox;
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
				// 形を変える取っ手（矢印よりさらに上。ギミックを1つだけ選んでいるときだけ）
				DrawHandles(drawList, stage, hoveredHandle);
			}

			// 範囲選択の四角（Windows のデスクトップのような半透明の青）
			if (m_isBoxSelecting)
			{
				const ImVec2 boxStartScreen = WorldToScreen(m_boxStart);
				const ImVec2 boxMin(math::Min(boxStartScreen.x, io.MousePos.x), math::Min(boxStartScreen.y, io.MousePos.y));
				const ImVec2 boxMax(math::Max(boxStartScreen.x, io.MousePos.x), math::Max(boxStartScreen.y, io.MousePos.y));
				drawList->AddRectFilled(boxMin, boxMax, EDITOR_COLOR_BOX_FILL);
				drawList->AddRect(boxMin, boxMax, EDITOR_COLOR_BOX_LINE);
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
			SelectSingle({ SelectionType::Gimmick, static_cast<int>(stage.GetGimmicks().GetCount()) - 1 });
		}


		void StageEditor::DuplicateSelected(Stage& stage)
		{
			if (m_selection.type != SelectionType::Gimmick || m_selectedGimmicks.empty())
			{
				return;
			}
			// 選択中の物を全部複製して、複製した物を選び直す（並び・主の物は元と同じにする）
			const std::vector<int> sources = m_selectedGimmicks;
			std::vector<int> copies;
			int primaryCopy = -1;
			for (const int sourceIndex : sources)
			{
				const GimmickBase* source = stage.GetGimmicks().Get(static_cast<size_t>(sourceIndex));
				if (source == nullptr)
				{
					continue;
				}
				// 名前の頭は追加ボタンと同じにする（"Door" → "Door3" など）
				const GimmickRegistryEntry* entry = FindGimmickRegistryEntry(source->GetTypeName());
				std::unique_ptr<GimmickBase> copy = source->Clone();
				copy->name = stage.GetGimmicks().MakeUniqueName(entry != nullptr ? entry->namePrefix : source->GetTypeName());
				copy->SetAnchor(source->GetAnchor() + math::Vec2(DUPLICATE_OFFSET, 0.0f));
				stage.GetGimmicks().Add(std::move(copy));

				const int copyIndex = static_cast<int>(stage.GetGimmicks().GetCount()) - 1;
				copies.push_back(copyIndex);
				if (sourceIndex == m_selection.index)
				{
					primaryCopy = copyIndex;
				}
			}
			if (copies.empty())
			{
				return;
			}
			m_selectedGimmicks = copies;
			m_selection = { SelectionType::Gimmick, primaryCopy >= 0 ? primaryCopy : copies.front() };
		}


		void StageEditor::RotateSelected(Stage& stage, const int quarterTurns)
		{
			// いくつか選んでいたら、それぞれをその場で回す
			// Undo の履歴には、UpdateHistory が変化を見つけて自動で積む（まとめて1回分になる）
			for (const int index : m_selectedGimmicks)
			{
				if (GimmickBase* gimmick = stage.GetGimmicks().Get(static_cast<size_t>(index)))
				{
					gimmick->Rotate90(quarterTurns);
				}
			}
		}


		void StageEditor::DeleteSelected(Stage& stage)
		{
			if (m_selection.type != SelectionType::Gimmick || m_selectedGimmicks.empty())
			{
				return;
			}
			GimmickManager& gimmicks = stage.GetGimmicks();

			// 後ろから消す（前から消すと、後ろの物の番号がずれるため）
			std::vector<int> indices = m_selectedGimmicks;
			std::sort(indices.begin(), indices.end(), [](const int a, const int b) { return a > b; });
			indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

			// 信号を出す物（受光器・スイッチなど）の名前を覚えておいて、消したあとに扉や霧のつながりからも外す
			std::vector<std::string> removedSignals;
			for (const int index : indices)
			{
				const GimmickBase* gimmick = gimmicks.Get(static_cast<size_t>(index));
				if (gimmick == nullptr)
				{
					continue;
				}
				if (gimmick->IsSignalSource())
				{
					removedSignals.push_back(gimmick->name);
				}
				gimmicks.Remove(static_cast<size_t>(index));
			}
			for (const std::string& name : removedSignals)
			{
				gimmicks.RemoveSignal(name);
			}

			SelectNone();
			m_isDraggingObject = false;
			m_isPendingSingleSelect = false;
			m_dragHandle = -1;
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


		/************************************/
		// 選択（いくつも選べる）


		void StageEditor::SelectNone()
		{
			m_selection = Selection();
			m_selectedGimmicks.clear();
		}


		void StageEditor::SelectSingle(const Selection& selection)
		{
			// selection が m_selection 自身のこともあるので、先に値を取っておく
			const Selection newSelection = selection;
			m_selection = newSelection;
			m_selectedGimmicks.clear();
			if (newSelection.type == SelectionType::Gimmick)
			{
				m_selectedGimmicks.push_back(newSelection.index);
			}
		}


		void StageEditor::ToggleGimmickSelection(const int index)
		{
			// 奥の鏡を選んでいたら外す（奥の鏡とギミックは一緒に選ばない）
			if (m_selection.type != SelectionType::Gimmick)
			{
				SelectNone();
			}

			const auto it = std::find(m_selectedGimmicks.begin(), m_selectedGimmicks.end(), index);
			if (it == m_selectedGimmicks.end())
			{
				// 足す（足した物を主にする）
				m_selectedGimmicks.push_back(index);
				m_selection = { SelectionType::Gimmick, index };
				return;
			}

			// 外す（主の物を外したら、残っている最後の物を主にする）
			m_selectedGimmicks.erase(it);
			if (m_selectedGimmicks.empty())
			{
				m_selection = Selection();
			}
			else if (m_selection.index == index)
			{
				m_selection.index = m_selectedGimmicks.back();
			}
		}


		bool StageEditor::IsGimmickSelected(const int index) const
		{
			return std::find(m_selectedGimmicks.begin(), m_selectedGimmicks.end(), index) != m_selectedGimmicks.end();
		}


		void StageEditor::SelectGimmicksInRect(const Stage& stage, const math::Rect& rect, const bool isAdditive)
		{
			if (!isAdditive || m_selection.type != SelectionType::Gimmick)
			{
				SelectNone();
			}

			// 四角に少しでもかかった物を選ぶ（全体が入っていなくてよい）
			const GimmickManager& gimmicks = stage.GetGimmicks();
			for (size_t i = 0; i < gimmicks.GetCount(); ++i)
			{
				const int index = static_cast<int>(i);
				if (!IsGimmickSelected(index) && IsTouchingBox(gimmicks.Get(i)->GetBounds(), rect))
				{
					m_selectedGimmicks.push_back(index);
				}
			}
			if (!m_selectedGimmicks.empty() && m_selection.type != SelectionType::Gimmick)
			{
				m_selection = { SelectionType::Gimmick, m_selectedGimmicks.front() };
			}
		}


		void StageEditor::SelectAll(const Stage& stage)
		{
			SelectNone();
			const int count = static_cast<int>(stage.GetGimmicks().GetCount());
			for (int i = 0; i < count; ++i)
			{
				m_selectedGimmicks.push_back(i);
			}
			if (!m_selectedGimmicks.empty())
			{
				m_selection = { SelectionType::Gimmick, m_selectedGimmicks.front() };
			}
		}


		void StageEditor::ValidateSelection(const Stage& stage)
		{
			// 消えた物（番号が範囲の外）を選択から外す
			const int count = static_cast<int>(stage.GetGimmicks().GetCount());
			m_selectedGimmicks.erase(
				std::remove_if(m_selectedGimmicks.begin(), m_selectedGimmicks.end(),
					[count](const int index) { return index < 0 || index >= count; }),
				m_selectedGimmicks.end());

			if (m_selection.type != SelectionType::Gimmick)
			{
				// 奥の鏡・選択なしのときは、ギミックの選択は空
				m_selectedGimmicks.clear();
				return;
			}
			if (!IsSelectionValid(stage))
			{
				// 主の物が消えた：残っている物があればそれを主にする
				if (m_selectedGimmicks.empty()) { m_selection = Selection(); }
				else { m_selection.index = m_selectedGimmicks.front(); }
				return;
			}
			if (!IsGimmickSelected(m_selection.index))
			{
				// 主の物が一覧に入っていない（念のため）：その物だけを選んでいることにする
				m_selectedGimmicks.assign(1, m_selection.index);
			}
		}


		void StageEditor::BeginObjectDrag(const Stage& stage)
		{
			m_isDraggingObject = m_selection.type != SelectionType::None;
			if (!m_isDraggingObject)
			{
				return;
			}
			m_dragStartAnchor = GetAnchor(stage, m_selection);
			m_dragAccumulated = math::Vec2::Zero();

			// 選択中のギミックそれぞれの最初の位置を覚えておく（同じだけ動かすため）
			m_dragStartAnchors.clear();
			for (const int index : m_selectedGimmicks)
			{
				const GimmickBase* gimmick = stage.GetGimmicks().Get(static_cast<size_t>(index));
				m_dragStartAnchors.push_back(gimmick != nullptr ? gimmick->GetAnchor() : math::Vec2::Zero());
			}
		}


		void StageEditor::MoveSelection(Stage& stage, const math::Vec2& anchor)
		{
			// 奥の鏡（ギミックを選んでいない）：今まで通り
			if (m_selection.type != SelectionType::Gimmick || m_dragStartAnchors.size() != m_selectedGimmicks.size())
			{
				SetAnchor(stage, m_selection, anchor);
				return;
			}

			// 主の物（つかんだ物）が動いた量だけ、全部を動かす（スナップは主の物で決めるので、並びはくずれない）
			const math::Vec2 moved = anchor - m_dragStartAnchor;
			for (size_t i = 0; i < m_selectedGimmicks.size(); ++i)
			{
				if (GimmickBase* gimmick = stage.GetGimmicks().Get(static_cast<size_t>(m_selectedGimmicks[i])))
				{
					gimmick->SetAnchor(m_dragStartAnchors[i] + moved);
				}
			}
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


		const GimmickBase* StageEditor::GetHandleOwner(const Stage& stage) const
		{
			// いくつか選んでいるときは、どれの形を変えるかわからないので出さない
			if (m_selection.type != SelectionType::Gimmick || m_selectedGimmicks.size() != 1 || !IsSelectionValid(stage))
			{
				return nullptr;
			}
			return stage.GetGimmicks().Get(static_cast<size_t>(m_selection.index));
		}


		int StageEditor::HitTestHandle(const Stage& stage, const ImVec2& mousePos) const
		{
			const GimmickBase* gimmick = GetHandleOwner(stage);
			if (gimmick == nullptr)
			{
				return -1;
			}
			// 後ろの取っ手ほど上に描くので、後ろから調べる
			for (int i = gimmick->GetEditHandleCount() - 1; i >= 0; --i)
			{
				const ImVec2 p = WorldToScreen(gimmick->GetEditHandle(i));
				if (std::abs(mousePos.x - p.x) <= HANDLE_HALF_PX + 2.0f && std::abs(mousePos.y - p.y) <= HANDLE_HALF_PX + 2.0f)
				{
					return i;
				}
			}
			return -1;
		}


		void StageEditor::DrawHandles(ImDrawList* drawList, const Stage& stage, const int hoveredHandle) const
		{
			const GimmickBase* gimmick = GetHandleOwner(stage);
			if (gimmick == nullptr)
			{
				return;
			}
			for (int i = 0; i < gimmick->GetEditHandleCount(); ++i)
			{
				// つかんでいる・マウスが乗っている取っ手は黄色にする
				const bool isHot = (m_isDraggingObject && m_dragHandle == i) || (!m_isDraggingObject && hoveredHandle == i);
				const ImVec2 p = WorldToScreen(gimmick->GetEditHandle(i));
				const ImVec2 a(p.x - HANDLE_HALF_PX, p.y - HANDLE_HALF_PX);
				const ImVec2 b(p.x + HANDLE_HALF_PX, p.y + HANDLE_HALF_PX);
				drawList->AddRectFilled(a, b, isHot ? EDITOR_COLOR_GIZMO_HOT : EDITOR_COLOR_HANDLE);
				drawList->AddRect(a, b, EDITOR_COLOR_HANDLE_BORDER);
			}
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
			ValidateSelection(stage);
			m_isDraggingObject = false;
			m_isPendingSingleSelect = false;
			m_dragAxis = GizmoAxis::None;
			m_dragHandle = -1;
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
			else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
			{
				SelectAll(stage);
			}
			else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
			{
				DuplicateSelected(stage);
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
			{
				// 選択中の物を全部消す（Undo で戻せる）
				DeleteSelected(stage);
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				SelectNone();
			}
			else if (!io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R, false))
			{
				// R：時計回り / Shift+R：反時計回り
				RotateSelected(stage, io.KeyShift ? 1 : -1);
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
			, m_canvasSize(0.0f, 0.0f)
			, m_inspectorWidth(INSPECTOR_DEFAULT_WIDTH)
			, m_isSnap(true)
			, m_gridSize(25.0f)
			, m_isShowLight(true)
			, m_isShowBackMirror(true)
			, m_isDraggingObject(false)
			, m_dragStartAnchor()
			, m_dragAccumulated()
			, m_dragAxis(GizmoAxis::None)
			, m_isPendingSingleSelect(false)
			, m_dragHandle(-1)
			, m_dragHandleStart()
			, m_isBoxSelecting(false)
			, m_boxStart()
			, m_isBoxAdditive(false)
		{}
	}
}
