/**
 * @file StageEditor.cpp
 * @brief ImGui のステージエディタ
 */
#include "stdafx.h"
#include "Source/Actor/Character/CharacterBase.h"
#include "Source/Stage/Stage.h"
#include "StageEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>


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
			/** 光源の表示半径（ピクセル） */
			constexpr float SOURCE_RADIUS_PX = 7.0f;
			/** 拡大率の範囲 */
			constexpr float MIN_ZOOM = 0.05f;
			constexpr float MAX_ZOOM = 4.0f;
			/** 複製したときにずらす量 */
			constexpr float DUPLICATE_OFFSET = 50.0f;
			/** 矢印の長さと先端の大きさ（ピクセル） */
			constexpr float GIZMO_LENGTH_PX = 60.0f;
			constexpr float GIZMO_HEAD_PX = 10.0f;

			const ImU32 EDITOR_COLOR_GIZMO_X = IM_COL32(235, 70, 70, 255);
			const ImU32 EDITOR_COLOR_GIZMO_Y = IM_COL32(90, 220, 90, 255);
			const ImU32 EDITOR_COLOR_GIZMO_HOT = IM_COL32(255, 230, 60, 255);

			// 色（COLOR_BACKGROUND などは Windows のヘッダー(winuser.h)がマクロで使っているので、EDITOR_ を付けている）
			const ImU32 EDITOR_COLOR_BACKGROUND = IM_COL32(28, 30, 36, 255);
			const ImU32 EDITOR_COLOR_GRID = IM_COL32(255, 255, 255, 18);
			const ImU32 EDITOR_COLOR_AXIS = IM_COL32(255, 255, 255, 50);
			const ImU32 EDITOR_COLOR_BLOCK = IM_COL32(120, 125, 140, 255);
			const ImU32 EDITOR_COLOR_BACK_MIRROR_FILL = IM_COL32(150, 120, 200, 22);
			const ImU32 EDITOR_COLOR_BACK_MIRROR = IM_COL32(190, 150, 255, 255);
			const ImU32 EDITOR_COLOR_DOOR = IM_COL32(170, 110, 60, 255);
			const ImU32 EDITOR_COLOR_PLATFORM_OFF = IM_COL32(90, 200, 230, 120);
			const ImU32 EDITOR_COLOR_PLATFORM_ON = IM_COL32(90, 200, 230, 200);
			const ImU32 EDITOR_COLOR_RECEIVER_OFF = IM_COL32(150, 140, 60, 255);
			const ImU32 EDITOR_COLOR_RECEIVER_ON = IM_COL32(255, 235, 90, 255);
			const ImU32 EDITOR_COLOR_SOURCE = IM_COL32(255, 160, 60, 255);
			const ImU32 EDITOR_COLOR_LIGHT_MIRROR = IM_COL32(230, 240, 255, 255);
			const ImU32 EDITOR_COLOR_LIGHT_MIRROR_BACK = IM_COL32(90, 90, 100, 255);
			const ImU32 EDITOR_COLOR_LIGHT = IM_COL32(255, 240, 120, 220);
			const ImU32 EDITOR_COLOR_RAIL = IM_COL32(230, 240, 255, 90);
			const ImU32 EDITOR_COLOR_CHARACTER = IM_COL32(110, 230, 120, 255);
			const ImU32 EDITOR_COLOR_SELECTED = IM_COL32(255, 90, 90, 255);
			const ImU32 EDITOR_COLOR_LABEL = IM_COL32(255, 255, 255, 170);
			const ImU32 EDITOR_COLOR_SWITCH_OFF = IM_COL32(230, 70, 60, 255);
			const ImU32 EDITOR_COLOR_SWITCH_ON = IM_COL32(90, 230, 110, 255);
			const ImU32 EDITOR_COLOR_MIST_FILL = IM_COL32(225, 235, 245, 14);
			const ImU32 EDITOR_COLOR_MIST = IM_COL32(225, 235, 245, 120);
			const ImU32 EDITOR_COLOR_MIRROR_PLATFORM = IM_COL32(200, 225, 245, 255);
			const ImU32 EDITOR_COLOR_MIRROR_PLATFORM_FILL = IM_COL32(200, 225, 245, 150);
			const ImU32 EDITOR_COLOR_CHECKPOINT = IM_COL32(110, 230, 120, 160);
			const ImU32 EDITOR_COLOR_WARP = IM_COL32(170, 120, 255, 255);
			const ImU32 EDITOR_COLOR_WARP_FILL = IM_COL32(60, 40, 110, 140);
			const ImU32 EDITOR_COLOR_INDICATOR_OFF = IM_COL32(90, 90, 100, 255);
			const ImU32 EDITOR_COLOR_INDICATOR_ON = IM_COL32(120, 255, 140, 255);
			const ImU32 EDITOR_COLOR_GOAL = IM_COL32(255, 200, 40, 255);


			/** 一覧に出す種類の名前 */
			const char* TypeLabel(const int type)
			{
				static const char* LABELS[] = {
					"None", "Blocks", "Light Sources", "Light Mirrors", "Receivers", "Doors", "Light Platforms", "Back Mirror",
					"Floor Switches", "Mist Emitters", "Mirror Platforms", "Checkpoints", "Warp Zones", "Indicators", "Goals" };
				return LABELS[type];
			}


			/** float を C++ のリテラルにする（100 → "100.0f"、12.5 → "12.5f"） */
			std::string FloatLiteral(const float value)
			{
				char buffer[32];
				std::snprintf(buffer, sizeof(buffer), "%.2f", value);
				std::string text = buffer;
				// 末尾の0を削る（ただし小数点の後ろは1桁残す）
				while (text.size() > 2 && text.back() == '0' && text[text.size() - 2] != '.')
				{
					text.pop_back();
				}
				if (text == "-0.0") { text = "0.0"; }
				return text + "f";
			}


			/** 文字列を C++ の文字列リテラルにする */
			std::string StringLiteral(const std::string& value)
			{
				std::string text = "\"";
				for (const char c : value)
				{
					if (c == '"' || c == '\\') { text += '\\'; }
					text += c;
				}
				return text + "\"";
			}


			std::string BoolLiteral(const bool value) { return value ? "true" : "false"; }


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


			/** 矩形の入力欄（左下とサイズ） */
			void EditRect(math::Rect& rect)
			{
				float position[2] = { rect.min.x, rect.min.y };
				float size[2] = { rect.Width(), rect.Height() };
				const bool isPositionChanged = ImGui::DragFloat2("Position", position, 1.0f, 0.0f, 0.0f, "%.1f");
				const bool isSizeChanged = ImGui::DragFloat2("Size", size, 1.0f, 1.0f, 100000.0f, "%.1f");
				if (isPositionChanged || isSizeChanged)
				{
					if (size[0] < 1.0f) { size[0] = 1.0f; }
					if (size[1] < 1.0f) { size[1] = 1.0f; }
					rect = math::Rect::FromMinSize(position[0], position[1], size[0], size[1]);
				}
			}


			/** 角度の入力欄（±ボタン付き） */
			void EditAngle(const char* label, float& angleDeg, const float stepDeg)
			{
				ImGui::PushID(label);
				if (ImGui::Button("-")) { angleDeg = math::NormalizeDeg(angleDeg - stepDeg); }
				ImGui::SameLine();
				if (ImGui::Button("+")) { angleDeg = math::NormalizeDeg(angleDeg + stepDeg); }
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-60.0f);
				if (ImGui::DragFloat(label, &angleDeg, 1.0f, 0.0f, 0.0f, "%.1f deg"))
				{
					angleDeg = math::NormalizeDeg(angleDeg);
				}
				ImGui::PopID();
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


			/** 点が矩形の中か */
			bool Contains(const math::Rect& rect, const math::Vec2& p)
			{
				return p.x >= rect.min.x && p.x <= rect.max.x && p.y >= rect.min.y && p.y <= rect.max.y;
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

			ImGui::SetNextWindowSize(ImVec2(1200.0f, 680.0f), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Stage Editor", &isOpen))
			{
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

			DrawExportWindow();
		}


		void StageEditor::AddCharacter(const char* label, const actor::CharacterBase* character)
		{
			m_characters.emplace_back(label, character);
		}


		std::string StageEditor::GenerateCppCode(const Stage& stage, const char* functionName) const
		{
			std::string code;
			code += "// StageEditor で出力したステージ定義\n";
			code += "void " + std::string(functionName) + "(app::stage::Stage& stage)\n{\n";
			code += "\tstage.Clear();\n";
			code += "\tstage.SetName(" + StringLiteral(stage.GetName()) + ");\n";
			const BackMirror& backMirror = stage.GetBackMirror();
			code += "\tstage.SetBackMirror(" + BoolLiteral(backMirror.isEnabled) + ", " + FloatLiteral(backMirror.planeZ) + ", "
				+ FloatLiteral(backMirror.frame.min.x) + ", " + FloatLiteral(backMirror.frame.min.y) + ", "
				+ FloatLiteral(backMirror.frame.Width()) + ", " + FloatLiteral(backMirror.frame.Height()) + ");\n";

			if (!stage.GetBlocks().empty())
			{
				code += "\n\t// 地形ブロック\n";
				for (const StageBlock& block : stage.GetBlocks())
				{
					code += "\tstage.AddBlock(" + StringLiteral(block.name) + ", "
						+ FloatLiteral(block.rect.min.x) + ", " + FloatLiteral(block.rect.min.y) + ", "
						+ FloatLiteral(block.rect.Width()) + ", " + FloatLiteral(block.rect.Height()) + ");\n";
				}
			}

			if (!stage.GetLightSources().empty())
			{
				code += "\n\t// 光源\n";
				for (const LightSource& source : stage.GetLightSources())
				{
					code += "\t{\n";
					code += "\t\tauto& source = stage.AddLightSource(" + StringLiteral(source.name) + ", "
						+ FloatLiteral(source.position.x) + ", " + FloatLiteral(source.position.y) + ", " + FloatLiteral(source.angleDeg) + ");\n";
					code += "\t\tsource.isOn = " + BoolLiteral(source.isOn) + ";\n";
					code += "\t}\n";
				}
			}

			if (!stage.GetLightMirrors().empty())
			{
				code += "\n\t// 反射鏡\n";
				for (const LightMirror& mirror : stage.GetLightMirrors())
				{
					code += "\t{\n";
					code += "\t\tauto& mirror = stage.AddLightMirror(" + StringLiteral(mirror.name) + ", "
						+ FloatLiteral(mirror.center.x) + ", " + FloatLiteral(mirror.center.y) + ", "
						+ FloatLiteral(mirror.angleDeg) + ", " + FloatLiteral(mirror.length) + ");\n";
					code += "\t\tmirror.isDoubleSided = " + BoolLiteral(mirror.isDoubleSided) + ";\n";
					code += "\t\tmirror.isRotatable = " + BoolLiteral(mirror.isRotatable) + ";\n";
					code += "\t\tmirror.rotationStepDeg = " + FloatLiteral(mirror.rotationStepDeg) + ";\n";
					code += "\t\tmirror.isMovable = " + BoolLiteral(mirror.isMovable) + ";\n";
					code += "\t\tmirror.hasRail = " + BoolLiteral(mirror.hasRail) + ";\n";
					code += "\t\tmirror.railMinX = " + FloatLiteral(mirror.railMinX) + ";\n";
					code += "\t\tmirror.railMaxX = " + FloatLiteral(mirror.railMaxX) + ";\n";
					code += "\t}\n";
				}
			}

			if (!stage.GetLightReceivers().empty())
			{
				code += "\n\t// 受光器\n";
				for (const LightReceiver& receiver : stage.GetLightReceivers())
				{
					code += "\tstage.AddLightReceiver(" + StringLiteral(receiver.name) + ", "
						+ FloatLiteral(receiver.rect.min.x) + ", " + FloatLiteral(receiver.rect.min.y) + ", "
						+ FloatLiteral(receiver.rect.Width()) + ", " + FloatLiteral(receiver.rect.Height()) + ");\n";
				}
			}

			if (!stage.GetDoors().empty())
			{
				code += "\n\t// 扉\n";
				for (const Door& door : stage.GetDoors())
				{
					std::string receivers = "{ ";
					for (size_t i = 0; i < door.requiredReceivers.size(); ++i)
					{
						receivers += (i > 0 ? ", " : "") + StringLiteral(door.requiredReceivers[i]);
					}
					receivers += door.requiredReceivers.empty() ? "}" : " }";

					code += "\t{\n";
					code += "\t\tauto& door = stage.AddDoor(" + StringLiteral(door.name) + ", "
						+ FloatLiteral(door.closedRect.min.x) + ", " + FloatLiteral(door.closedRect.min.y) + ", "
						+ FloatLiteral(door.closedRect.Width()) + ", " + FloatLiteral(door.closedRect.Height()) + ", " + receivers + ");\n";
					code += "\t\tdoor.openSpeed = " + FloatLiteral(door.openSpeed) + ";\n";
					code += "\t\tdoor.isLatch = " + BoolLiteral(door.isLatch) + ";\n";
					code += "\t}\n";
				}
			}

			if (!stage.GetLightPlatforms().empty())
			{
				code += "\n\t// 光の足場\n";
				for (const LightPlatform& platform : stage.GetLightPlatforms())
				{
					code += "\t{\n";
					code += "\t\tauto& platform = stage.AddLightPlatform(" + StringLiteral(platform.name) + ", "
						+ FloatLiteral(platform.rect.min.x) + ", " + FloatLiteral(platform.rect.min.y) + ", "
						+ FloatLiteral(platform.rect.Width()) + ", " + FloatLiteral(platform.rect.Height()) + ");\n";
					code += "\t\tplatform.lingerTime = " + FloatLiteral(platform.lingerTime) + ";\n";
					code += "\t}\n";
				}
			}

			if (!stage.GetCheckpoints().empty())
			{
				code += "\n\t// チェックポイント\n";
				for (const Checkpoint& checkpoint : stage.GetCheckpoints())
				{
					code += "\tstage.AddCheckpoint(" + StringLiteral(checkpoint.name) + ", "
						+ FloatLiteral(checkpoint.rect.min.x) + ", " + FloatLiteral(checkpoint.rect.min.y) + ", "
						+ FloatLiteral(checkpoint.rect.Width()) + ", " + FloatLiteral(checkpoint.rect.Height()) + ");\n";
				}
			}

			if (!stage.GetFloorSwitches().empty())
			{
				code += "\n\t// 床スイッチ\n";
				for (const FloorSwitch& floorSwitch : stage.GetFloorSwitches())
				{
					code += "\t{\n";
					code += "\t\tauto& floorSwitch = stage.AddFloorSwitch(" + StringLiteral(floorSwitch.name) + ", "
						+ FloatLiteral(floorSwitch.rect.min.x) + ", " + FloatLiteral(floorSwitch.rect.min.y) + ", "
						+ FloatLiteral(floorSwitch.rect.Width()) + ", " + FloatLiteral(floorSwitch.rect.Height()) + ");\n";
					code += "\t\tfloorSwitch.isLatch = " + BoolLiteral(floorSwitch.isLatch) + ";\n";
					code += "\t}\n";
				}
			}

			if (!stage.GetMistEmitters().empty())
			{
				code += "\n\t// 霧の発生装置\n";
				for (const MistEmitter& mist : stage.GetMistEmitters())
				{
					code += "\t{\n";
					code += "\t\tauto& mist = stage.AddMistEmitter(" + StringLiteral(mist.name) + ", "
						+ FloatLiteral(mist.area.min.x) + ", " + FloatLiteral(mist.area.min.y) + ", "
						+ FloatLiteral(mist.area.Width()) + ", " + FloatLiteral(mist.area.Height()) + ", " + StringLiteral(mist.triggerSignal) + ");\n";
					code += "\t\tmist.fallSpeed = " + FloatLiteral(mist.fallSpeed) + ";\n";
					code += "\t\tmist.duration = " + FloatLiteral(mist.duration) + ";\n";
					code += "\t\tmist.fadeTime = " + FloatLiteral(mist.fadeTime) + ";\n";
					code += "\t\tmist.ventSpacing = " + FloatLiteral(mist.ventSpacing) + ";\n";
					code += "\t}\n";
				}
			}

			if (!stage.GetMirrorPlatforms().empty())
			{
				code += "\n\t// 鏡の足場\n";
				for (const MirrorPlatform& platform : stage.GetMirrorPlatforms())
				{
					code += "\t{\n";
					code += "\t\tauto& platform = stage.AddMirrorPlatform(" + StringLiteral(platform.name) + ", "
						+ FloatLiteral(platform.rect.min.x) + ", " + FloatLiteral(platform.rect.min.y) + ", "
						+ FloatLiteral(platform.rect.Width()) + ", " + FloatLiteral(platform.rect.Height()) + ");\n";
					code += "\t\tplatform.isSolidWhenHidden = " + BoolLiteral(platform.isSolidWhenHidden) + ";\n";
					code += "\t}\n";
				}
			}

			if (!stage.GetIndicators().empty())
			{
				code += "\n\t// 合図のランプ\n";
				for (const Indicator& indicator : stage.GetIndicators())
				{
					code += "\tstage.AddIndicator(" + StringLiteral(indicator.name) + ", "
						+ FloatLiteral(indicator.rect.min.x) + ", " + FloatLiteral(indicator.rect.min.y) + ", "
						+ FloatLiteral(indicator.rect.Width()) + ", " + FloatLiteral(indicator.rect.Height()) + ", " + StringLiteral(indicator.signal) + ");\n";
				}
			}

			if (!stage.GetWarpZones().empty())
			{
				code += "\n\t// ワープ\n";
				for (const WarpZone& zone : stage.GetWarpZones())
				{
					code += "\t{\n";
					code += "\t\tauto& warp = stage.AddWarpZone(" + StringLiteral(zone.name) + ", "
						+ FloatLiteral(zone.rect.min.x) + ", " + FloatLiteral(zone.rect.min.y) + ", "
						+ FloatLiteral(zone.rect.Width()) + ", " + FloatLiteral(zone.rect.Height()) + ", "
						+ FloatLiteral(zone.destination.x) + ", " + FloatLiteral(zone.destination.y) + ");\n";
					code += "\t\twarp.disabledBySignal = " + StringLiteral(zone.disabledBySignal) + ";\n";
					code += "\t}\n";
				}
			}

			if (!stage.GetGoals().empty())
			{
				code += "\n\t// ゴール\n";
				for (const Goal& goal : stage.GetGoals())
				{
					code += "\tstage.AddGoal(" + StringLiteral(goal.name) + ", "
						+ FloatLiteral(goal.rect.min.x) + ", " + FloatLiteral(goal.rect.min.y) + ", "
						+ FloatLiteral(goal.rect.Width()) + ", " + FloatLiteral(goal.rect.Height()) + ");\n";
				}
			}

			code += "}\n";
			return code;
		}


		/************************************/
		// 上部のツールバー


		void StageEditor::DrawToolbar(Stage& stage)
		{
			if (ImGui::Button("Export C++"))
			{
				m_exportText = GenerateCppCode(stage, m_functionName);
				ImGui::SetClipboardText(m_exportText.c_str());
				m_isShowExport = true;
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(160.0f);
			ImGui::InputText("Function", m_functionName, sizeof(m_functionName));
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
			if (ImGui::Button("Fit View"))
			{
				// 全ての地形が見えるように（縦に長いステージでも収まるよう、幅と高さの両方で合わせる）
				if (!stage.GetBlocks().empty())
				{
					math::Rect bounds = stage.GetBlocks().front().rect;
					for (const StageBlock& block : stage.GetBlocks())
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

			// 追加ボタン
			ImGui::TextUnformatted("Add:");
			ImGui::SameLine(); if (ImGui::SmallButton("Block")) { AddObject(stage, ObjectType::Block); }
			ImGui::SameLine(); if (ImGui::SmallButton("Light Source")) { AddObject(stage, ObjectType::LightSource); }
			ImGui::SameLine(); if (ImGui::SmallButton("Light Mirror")) { AddObject(stage, ObjectType::LightMirror); }
			ImGui::SameLine(); if (ImGui::SmallButton("Receiver")) { AddObject(stage, ObjectType::LightReceiver); }
			ImGui::SameLine(); if (ImGui::SmallButton("Door")) { AddObject(stage, ObjectType::Door); }
			ImGui::SameLine(); if (ImGui::SmallButton("Light Platform")) { AddObject(stage, ObjectType::LightPlatform); }
			ImGui::SameLine(); if (ImGui::SmallButton("Switch")) { AddObject(stage, ObjectType::FloorSwitch); }
			ImGui::SameLine(); if (ImGui::SmallButton("Mist")) { AddObject(stage, ObjectType::MistEmitter); }
			ImGui::SameLine(); if (ImGui::SmallButton("Mirror Platform")) { AddObject(stage, ObjectType::MirrorPlatform); }
			ImGui::SameLine(); if (ImGui::SmallButton("Checkpoint")) { AddObject(stage, ObjectType::Checkpoint); }
			ImGui::SameLine(); if (ImGui::SmallButton("Warp")) { AddObject(stage, ObjectType::WarpZone); }
			ImGui::SameLine(); if (ImGui::SmallButton("Lamp")) { AddObject(stage, ObjectType::Indicator); }
			ImGui::SameLine(); if (ImGui::SmallButton("Goal")) { AddObject(stage, ObjectType::Goal); }
			ImGui::SameLine(); ImGui::TextDisabled("|");
			ImGui::SameLine();
			ImGui::BeginDisabled(m_selection.type == ObjectType::None || m_selection.type == ObjectType::BackMirror);
			if (ImGui::SmallButton("Duplicate")) { DuplicateSelected(stage); }
			ImGui::SameLine();
			if (ImGui::SmallButton("Delete")) { DeleteSelected(stage); }
			ImGui::EndDisabled();
		}


		/************************************/
		// 物の一覧


		void StageEditor::DrawObjectList(Stage& stage)
		{
			ImGui::TextUnformatted(stage.GetName().empty() ? "(no name)" : stage.GetName().c_str());

			// 1種類ぶんの一覧を出す
			auto drawList = [this](const ObjectType type, const int count, auto getName)
				{
					char header[64];
					std::snprintf(header, sizeof(header), "%s (%d)###list%d", TypeLabel(static_cast<int>(type)), count, static_cast<int>(type));
					if (ImGui::TreeNodeEx(header, ImGuiTreeNodeFlags_DefaultOpen))
					{
						for (int i = 0; i < count; ++i)
						{
							ImGui::PushID(i);
							const bool isSelected = m_selection.type == type && m_selection.index == i;
							if (ImGui::Selectable(getName(i), isSelected))
							{
								m_selection = { type, i };
							}
							ImGui::PopID();
						}
						ImGui::TreePop();
					}
				};

			if (ImGui::Selectable("Back Mirror", m_selection.type == ObjectType::BackMirror))
			{
				m_selection = { ObjectType::BackMirror, 0 };
			}
			drawList(ObjectType::Block, static_cast<int>(stage.GetBlocks().size()), [&](int i) { return stage.GetBlocks()[i].name.c_str(); });
			drawList(ObjectType::LightSource, static_cast<int>(stage.GetLightSources().size()), [&](int i) { return stage.GetLightSources()[i].name.c_str(); });
			drawList(ObjectType::LightMirror, static_cast<int>(stage.GetLightMirrors().size()), [&](int i) { return stage.GetLightMirrors()[i].name.c_str(); });
			drawList(ObjectType::LightReceiver, static_cast<int>(stage.GetLightReceivers().size()), [&](int i) { return stage.GetLightReceivers()[i].name.c_str(); });
			drawList(ObjectType::Door, static_cast<int>(stage.GetDoors().size()), [&](int i) { return stage.GetDoors()[i].name.c_str(); });
			drawList(ObjectType::LightPlatform, static_cast<int>(stage.GetLightPlatforms().size()), [&](int i) { return stage.GetLightPlatforms()[i].name.c_str(); });
			drawList(ObjectType::FloorSwitch, static_cast<int>(stage.GetFloorSwitches().size()), [&](int i) { return stage.GetFloorSwitches()[i].name.c_str(); });
			drawList(ObjectType::MistEmitter, static_cast<int>(stage.GetMistEmitters().size()), [&](int i) { return stage.GetMistEmitters()[i].name.c_str(); });
			drawList(ObjectType::MirrorPlatform, static_cast<int>(stage.GetMirrorPlatforms().size()), [&](int i) { return stage.GetMirrorPlatforms()[i].name.c_str(); });
			drawList(ObjectType::Checkpoint, static_cast<int>(stage.GetCheckpoints().size()), [&](int i) { return stage.GetCheckpoints()[i].name.c_str(); });
			drawList(ObjectType::WarpZone, static_cast<int>(stage.GetWarpZones().size()), [&](int i) { return stage.GetWarpZones()[i].name.c_str(); });
			drawList(ObjectType::Indicator, static_cast<int>(stage.GetIndicators().size()), [&](int i) { return stage.GetIndicators()[i].name.c_str(); });
			drawList(ObjectType::Goal, static_cast<int>(stage.GetGoals().size()), [&](int i) { return stage.GetGoals()[i].name.c_str(); });
		}


		/************************************/
		// 選択中の物の数値


		void StageEditor::DrawProperties(Stage& stage)
		{
			const int index = m_selection.index;
			ImGui::PushID("Properties");

			switch (m_selection.type)
			{
			case ObjectType::None:
				ImGui::TextDisabled("Select an object.");
				break;

			case ObjectType::BackMirror:
			{
				BackMirror& backMirror = stage.GetBackMirror();
				ImGui::Checkbox("Enabled", &backMirror.isEnabled);
				ImGui::DragFloat("Plane Z", &backMirror.planeZ, 1.0f, -5000.0f, 5000.0f, "%.1f");
				ImGui::SetItemTooltip("Depth of the mirror surface. Match it to the back edge of the floor.");
				EditRect(backMirror.frame);
				ImGui::TextWrapped("The mirror on the back wall. It reflects the room and the player (visual only).");
				break;
			}

			case ObjectType::Block:
			{
				StageBlock& block = stage.GetBlocks()[index];
				EditName(block.name);
				EditRect(block.rect);
				break;
			}

			case ObjectType::LightSource:
			{
				LightSource& source = stage.GetLightSources()[index];
				EditName(source.name);
				ImGui::DragFloat2("Position", &source.position.x, 1.0f, 0.0f, 0.0f, "%.1f");
				EditAngle("Angle", source.angleDeg, 45.0f);
				ImGui::Checkbox("On", &source.isOn);
				break;
			}

			case ObjectType::LightMirror:
			{
				LightMirror& mirror = stage.GetLightMirrors()[index];
				EditName(mirror.name);
				ImGui::DragFloat2("Center", &mirror.center.x, 1.0f, 0.0f, 0.0f, "%.1f");
				EditAngle("Angle", mirror.angleDeg, mirror.rotationStepDeg);
				ImGui::DragFloat("Length", &mirror.length, 1.0f, 5.0f, 2000.0f, "%.1f");
				ImGui::Checkbox("Double Sided", &mirror.isDoubleSided);
				ImGui::Separator();
				ImGui::Checkbox("Rotatable", &mirror.isRotatable);
				ImGui::DragFloat("Rotation Step", &mirror.rotationStepDeg, 1.0f, 1.0f, 180.0f, "%.0f deg");
				ImGui::Checkbox("Movable", &mirror.isMovable);
				ImGui::Checkbox("Rail", &mirror.hasRail);
				if (mirror.hasRail)
				{
					ImGui::DragFloat("Rail Min X", &mirror.railMinX, 1.0f, 0.0f, 0.0f, "%.1f");
					ImGui::DragFloat("Rail Max X", &mirror.railMaxX, 1.0f, 0.0f, 0.0f, "%.1f");
					if (mirror.railMaxX < mirror.railMinX) { mirror.railMaxX = mirror.railMinX; }
				}
				break;
			}

			case ObjectType::LightReceiver:
			{
				LightReceiver& receiver = stage.GetLightReceivers()[index];
				const std::string oldName = receiver.name;
				if (EditName(receiver.name))
				{
					// 名前を変えたら、扉と霧のつながりも新しい名前に付け替える
					RenameSignal(stage, oldName, receiver.name);
				}
				EditRect(receiver.rect);
				ImGui::Text("Lit: %s", receiver.isLit ? "YES" : "no");
				break;
			}

			case ObjectType::Door:
			{
				Door& door = stage.GetDoors()[index];
				EditName(door.name);
				EditRect(door.closedRect);
				ImGui::DragFloat("Open Speed", &door.openSpeed, 0.05f, 0.05f, 20.0f, "%.2f");
				ImGui::Checkbox("Latch (stay open)", &door.isLatch);
				ImGui::Text("Open: %.0f%%", door.openAmount * 100.0f);
				ImGui::TextUnformatted("Required signals (receivers / switches):");
				auto signalCheckbox = [&door](const std::string& name)
					{
						auto it = std::find(door.requiredReceivers.begin(), door.requiredReceivers.end(), name);
						bool isRequired = it != door.requiredReceivers.end();
						if (ImGui::Checkbox(name.c_str(), &isRequired))
						{
							if (isRequired) { door.requiredReceivers.push_back(name); }
							else { door.requiredReceivers.erase(it); }
						}
					};
				for (const LightReceiver& receiver : stage.GetLightReceivers()) { signalCheckbox(receiver.name); }
				for (const FloorSwitch& floorSwitch : stage.GetFloorSwitches()) { signalCheckbox(floorSwitch.name); }
				if (stage.GetLightReceivers().empty() && stage.GetFloorSwitches().empty())
				{
					ImGui::TextDisabled("(no receivers / switches)");
				}
				break;
			}

			case ObjectType::LightPlatform:
			{
				LightPlatform& platform = stage.GetLightPlatforms()[index];
				EditName(platform.name);
				EditRect(platform.rect);
				ImGui::DragFloat("Linger Time", &platform.lingerTime, 0.01f, 0.0f, 5.0f, "%.2f s");
				ImGui::Text("Solid: %s", platform.IsSolid() ? "YES" : "no");
				break;
			}

			case ObjectType::FloorSwitch:
			{
				FloorSwitch& floorSwitch = stage.GetFloorSwitches()[index];
				const std::string oldName = floorSwitch.name;
				if (EditName(floorSwitch.name))
				{
					RenameSignal(stage, oldName, floorSwitch.name);
				}
				EditRect(floorSwitch.rect);
				ImGui::SetItemTooltip("Put it on top of a block. It has no collision.");
				ImGui::Checkbox("Latch (stay pressed)", &floorSwitch.isLatch);
				ImGui::Text("Pressed: %s", floorSwitch.isPressed ? "YES" : "no");
				break;
			}

			case ObjectType::MistEmitter:
			{
				MistEmitter& mist = stage.GetMistEmitters()[index];
				EditName(mist.name);
				EditRect(mist.area);
				ImGui::SetItemTooltip("The mist spreads down from the top of this area (the ceiling).");
				// きっかけの信号を受光器・スイッチから選ぶ
				if (ImGui::BeginCombo("Trigger", mist.triggerSignal.empty() ? "(none)" : mist.triggerSignal.c_str()))
				{
					if (ImGui::Selectable("(none)", mist.triggerSignal.empty())) { mist.triggerSignal.clear(); }
					for (const FloorSwitch& floorSwitch : stage.GetFloorSwitches())
					{
						if (ImGui::Selectable(floorSwitch.name.c_str(), mist.triggerSignal == floorSwitch.name)) { mist.triggerSignal = floorSwitch.name; }
					}
					for (const LightReceiver& receiver : stage.GetLightReceivers())
					{
						if (ImGui::Selectable(receiver.name.c_str(), mist.triggerSignal == receiver.name)) { mist.triggerSignal = receiver.name; }
					}
					ImGui::EndCombo();
				}
				ImGui::DragFloat("Fall Speed", &mist.fallSpeed, 5.0f, 1.0f, 10000.0f, "%.0f");
				ImGui::DragFloat("Duration", &mist.duration, 0.05f, 0.0f, 60.0f, "%.2f s");
				ImGui::DragFloat("Fade Time", &mist.fadeTime, 0.05f, 0.0f, 60.0f, "%.2f s");
				ImGui::DragFloat("Vent Spacing", &mist.ventSpacing, 1.0f, 20.0f, 5000.0f, "%.0f");
				ImGui::Text("Fired: %s  Time: %.2f  Density: %.2f", mist.hasFired ? "YES" : "no", mist.timer, mist.GetDensity());
				if (ImGui::SmallButton("Fire now"))
				{
					mist.hasFired = true;
					mist.timer = 0.0f;
				}
				ImGui::SetItemTooltip("Blow the mist without the trigger (for testing). Reset Runtime to undo.");
				break;
			}

			case ObjectType::MirrorPlatform:
			{
				MirrorPlatform& platform = stage.GetMirrorPlatforms()[index];
				EditName(platform.name);
				EditRect(platform.rect);
				ImGui::Checkbox("Solid when hidden", &platform.isSolidWhenHidden);
				ImGui::SetItemTooltip("ON: you can stand on it before the mist reveals it.");
				ImGui::Checkbox("Revealed", &platform.isRevealed);
				ImGui::Text("Solid: %s", platform.IsSolid() ? "YES" : "no");
				break;
			}

			case ObjectType::Checkpoint:
			{
				Checkpoint& checkpoint = stage.GetCheckpoints()[index];
				EditName(checkpoint.name);
				EditRect(checkpoint.rect);
				const math::Vec2 spawn = checkpoint.GetSpawnPosition();
				ImGui::Text("Respawn at (%.0f, %.0f)", spawn.x, spawn.y);
				ImGui::TextWrapped("Touch this area to respawn here. Put the bottom on the floor.");
				break;
			}

			case ObjectType::WarpZone:
			{
				WarpZone& zone = stage.GetWarpZones()[index];
				EditName(zone.name);
				EditRect(zone.rect);
				ImGui::DragFloat2("Destination", &zone.destination.x, 1.0f, 0.0f, 0.0f, "%.1f");
				ImGui::SetItemTooltip("Feet position after the warp. Keep it outside every warp zone.");
				SignalCombo(stage, "Disabled by", zone.disabledBySignal);
				ImGui::SetItemTooltip("While this signal is ON, the warp does nothing.");
				ImGui::TextWrapped("The player warps when the center of the body enters this area. The running speed is kept.");
				break;
			}

			case ObjectType::Indicator:
			{
				Indicator& indicator = stage.GetIndicators()[index];
				EditName(indicator.name);
				EditRect(indicator.rect);
				SignalCombo(stage, "Signal", indicator.signal);
				ImGui::Text("On: %s", indicator.isOn ? "YES" : "no");
				ImGui::TextDisabled("(none) = a dummy lamp that never lights.");
				break;
			}

			case ObjectType::Goal:
			{
				Goal& goal = stage.GetGoals()[index];
				EditName(goal.name);
				EditRect(goal.rect);
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
				m_isDraggingObject = m_selection.type != ObjectType::None;
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
			auto isSelected = [&](const ObjectType type, const size_t i)
				{
					return m_selection.type == type && m_selection.index == static_cast<int>(i);
				};

			// 奥の鏡（部屋の奥の壁。いちばん後ろに描く）
			const BackMirror& backMirror = stage.GetBackMirror();
			if (backMirror.isEnabled && m_isShowBackMirror)
			{
				drawRect(backMirror.frame, EDITOR_COLOR_BACK_MIRROR_FILL, 0);
			}

			// 地形ブロック
			for (size_t i = 0; i < stage.GetBlocks().size(); ++i)
			{
				const StageBlock& block = stage.GetBlocks()[i];
				drawRect(block.rect, EDITOR_COLOR_BLOCK, isSelected(ObjectType::Block, i) ? EDITOR_COLOR_SELECTED : 0);
			}
			// 光の足場
			for (size_t i = 0; i < stage.GetLightPlatforms().size(); ++i)
			{
				const LightPlatform& platform = stage.GetLightPlatforms()[i];
				drawRect(platform.rect, platform.IsSolid() ? EDITOR_COLOR_PLATFORM_ON : 0,
					isSelected(ObjectType::LightPlatform, i) ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_PLATFORM_OFF);
				drawLabel({ platform.rect.min.x, platform.rect.max.y }, platform.name);
			}
			// 扉（閉じている範囲を枠で、今の範囲を塗りで）
			for (size_t i = 0; i < stage.GetDoors().size(); ++i)
			{
				const Door& door = stage.GetDoors()[i];
				if (door.IsSolid()) { drawRect(door.GetCurrentRect(), EDITOR_COLOR_DOOR, 0); }
				drawRect(door.closedRect, 0, isSelected(ObjectType::Door, i) ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_DOOR);
				drawLabel({ door.closedRect.min.x, door.closedRect.max.y }, door.name);
			}
			// 霧の範囲（薄い塗りと枠。先端の線は霧が出ている間だけ）
			for (size_t i = 0; i < stage.GetMistEmitters().size(); ++i)
			{
				const MistEmitter& mist = stage.GetMistEmitters()[i];
				drawRect(mist.area, EDITOR_COLOR_MIST_FILL, isSelected(ObjectType::MistEmitter, i) ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_MIST);
				if (mist.IsActive())
				{
					const float frontY = mist.GetFrontY();
					drawList->AddLine(WorldToScreen({ mist.area.min.x, frontY }), WorldToScreen({ mist.area.max.x, frontY }), EDITOR_COLOR_MIST, 2.0f);
				}
				drawLabel({ mist.area.min.x, mist.area.max.y }, mist.name + (mist.triggerSignal.empty() ? "" : " <- " + mist.triggerSignal));
			}
			// 鏡の足場（見えるようになったら塗る）
			for (size_t i = 0; i < stage.GetMirrorPlatforms().size(); ++i)
			{
				const MirrorPlatform& platform = stage.GetMirrorPlatforms()[i];
				drawRect(platform.rect, platform.isRevealed ? EDITOR_COLOR_MIRROR_PLATFORM_FILL : 0,
					isSelected(ObjectType::MirrorPlatform, i) ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_MIRROR_PLATFORM);
				drawLabel({ platform.rect.min.x, platform.rect.max.y }, platform.name);
			}
			// 床スイッチ
			for (size_t i = 0; i < stage.GetFloorSwitches().size(); ++i)
			{
				const FloorSwitch& floorSwitch = stage.GetFloorSwitches()[i];
				drawRect(floorSwitch.rect, floorSwitch.isPressed ? EDITOR_COLOR_SWITCH_ON : EDITOR_COLOR_SWITCH_OFF,
					isSelected(ObjectType::FloorSwitch, i) ? EDITOR_COLOR_SELECTED : 0);
				drawLabel({ floorSwitch.rect.min.x, floorSwitch.rect.max.y + 20.0f / m_zoom }, floorSwitch.name);
			}
			// チェックポイント（再開位置に印）
			for (size_t i = 0; i < stage.GetCheckpoints().size(); ++i)
			{
				const Checkpoint& checkpoint = stage.GetCheckpoints()[i];
				drawRect(checkpoint.rect, 0, isSelected(ObjectType::Checkpoint, i) ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_CHECKPOINT);
				drawList->AddCircleFilled(WorldToScreen(checkpoint.GetSpawnPosition()), 4.0f, EDITOR_COLOR_CHECKPOINT);
				drawLabel({ checkpoint.rect.min.x, checkpoint.rect.max.y }, checkpoint.name);
			}
			// ワープ（行き先まで線を引く）
			for (size_t i = 0; i < stage.GetWarpZones().size(); ++i)
			{
				const WarpZone& zone = stage.GetWarpZones()[i];
				const bool isSelectedZone = isSelected(ObjectType::WarpZone, i);
				drawRect(zone.rect, EDITOR_COLOR_WARP_FILL, isSelectedZone ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_WARP);
				const math::Vec2 center = (zone.rect.min + zone.rect.max) * 0.5f;
				drawList->AddLine(WorldToScreen(center), WorldToScreen(zone.destination), isSelectedZone ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_WARP, isSelectedZone ? 2.0f : 1.0f);
				drawList->AddCircleFilled(WorldToScreen(zone.destination), 4.0f, EDITOR_COLOR_WARP);
				drawLabel({ zone.rect.min.x, zone.rect.max.y }, zone.name);
			}
			// 合図のランプ
			for (size_t i = 0; i < stage.GetIndicators().size(); ++i)
			{
				const Indicator& indicator = stage.GetIndicators()[i];
				drawRect(indicator.rect, indicator.isOn ? EDITOR_COLOR_INDICATOR_ON : EDITOR_COLOR_INDICATOR_OFF,
					isSelected(ObjectType::Indicator, i) ? EDITOR_COLOR_SELECTED : 0);
				drawLabel({ indicator.rect.min.x, indicator.rect.max.y }, indicator.name + (indicator.signal.empty() ? "" : " <- " + indicator.signal));
			}
			// ゴール
			for (size_t i = 0; i < stage.GetGoals().size(); ++i)
			{
				const Goal& goal = stage.GetGoals()[i];
				drawRect(goal.rect, 0, isSelected(ObjectType::Goal, i) ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_GOAL);
				drawLabel({ goal.rect.min.x, goal.rect.max.y }, goal.name);
			}
			// 受光器
			for (size_t i = 0; i < stage.GetLightReceivers().size(); ++i)
			{
				const LightReceiver& receiver = stage.GetLightReceivers()[i];
				drawRect(receiver.rect, receiver.isLit ? EDITOR_COLOR_RECEIVER_ON : EDITOR_COLOR_RECEIVER_OFF,
					isSelected(ObjectType::LightReceiver, i) ? EDITOR_COLOR_SELECTED : 0);
				drawLabel({ receiver.rect.min.x, receiver.rect.max.y }, receiver.name);
			}

			// 光
			if (m_isShowLight)
			{
				for (const LightSegment& segment : stage.GetLightResult().segments)
				{
					drawList->AddLine(WorldToScreen(segment.start), WorldToScreen(segment.end), EDITOR_COLOR_LIGHT, 2.0f);
				}
			}

			// 反射鏡（表を明るく、裏を暗く。中心から表側に短い線）
			for (size_t i = 0; i < stage.GetLightMirrors().size(); ++i)
			{
				const LightMirror& mirror = stage.GetLightMirrors()[i];
				const bool isSelectedMirror = isSelected(ObjectType::LightMirror, i);
				if (mirror.hasRail)
				{
					const ImVec2 a = WorldToScreen({ mirror.railMinX, mirror.center.y });
					const ImVec2 b = WorldToScreen({ mirror.railMaxX, mirror.center.y });
					drawList->AddLine(a, b, EDITOR_COLOR_RAIL, 1.0f);
				}
				const math::Vec2 backOffset = mirror.GetNormal() * (-2.0f / m_zoom);
				drawList->AddLine(WorldToScreen(mirror.GetStart() + backOffset), WorldToScreen(mirror.GetEnd() + backOffset),
					mirror.isDoubleSided ? EDITOR_COLOR_LIGHT_MIRROR : EDITOR_COLOR_LIGHT_MIRROR_BACK, 3.0f);
				drawList->AddLine(WorldToScreen(mirror.GetStart()), WorldToScreen(mirror.GetEnd()),
					isSelectedMirror ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_LIGHT_MIRROR, 3.0f);
				drawList->AddLine(WorldToScreen(mirror.center), WorldToScreen(mirror.center + mirror.GetNormal() * (12.0f / m_zoom)),
					isSelectedMirror ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_LIGHT_MIRROR, 1.0f);
				drawLabel(mirror.GetEnd(), mirror.name);
			}

			// 光源
			for (size_t i = 0; i < stage.GetLightSources().size(); ++i)
			{
				const LightSource& source = stage.GetLightSources()[i];
				const ImVec2 p = WorldToScreen(source.position);
				const ImU32 color = isSelected(ObjectType::LightSource, i) ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_SOURCE;
				drawList->AddCircleFilled(p, SOURCE_RADIUS_PX, source.isOn ? color : EDITOR_COLOR_LIGHT_MIRROR_BACK);
				drawList->AddLine(p, WorldToScreen(source.position + math::DirectionFromDeg(source.angleDeg) * (20.0f / m_zoom)), color, 2.0f);
				drawLabel(source.position, source.name);
			}

			// 奥の鏡の枠（枠線をクリックすると選択できる）
			if (backMirror.isEnabled && m_isShowBackMirror)
			{
				const bool isSelectedFrame = m_selection.type == ObjectType::BackMirror;
				drawRect(backMirror.frame, 0, isSelectedFrame ? EDITOR_COLOR_SELECTED : EDITOR_COLOR_BACK_MIRROR);
				drawLabel({ backMirror.frame.min.x, backMirror.frame.max.y }, "Back Mirror");
			}

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
			if (m_selection.type != ObjectType::None)
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

		void StageEditor::DrawExportWindow()
		{
			if (!m_isShowExport)
			{
				return;
			}
			ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Stage C++ Code", &m_isShowExport))
			{
				ImGui::TextUnformatted("Copied to clipboard. Paste it into your stage definition (.cpp).");
				if (ImGui::Button("Copy Again"))
				{
					ImGui::SetClipboardText(m_exportText.c_str());
				}
				// 読み取り専用なので書き換えられないが、引数が char* なので &[0] で渡す（C++14 では data() が const char* を返すため）
				ImGui::InputTextMultiline("##code", &m_exportText[0], m_exportText.size() + 1,
					ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
			}
			ImGui::End();
		}


		/************************************/
		// 編集操作


		void StageEditor::AddObject(Stage& stage, const ObjectType type)
		{
			const float x = Snap(m_viewCenter.x);
			const float y = Snap(m_viewCenter.y);
			switch (type)
			{
			case ObjectType::Block:
				stage.AddBlock(MakeUniqueName(stage, "Block").c_str(), x, y, 200.0f, 50.0f);
				m_selection = { type, static_cast<int>(stage.GetBlocks().size()) - 1 };
				break;
			case ObjectType::LightSource:
				stage.AddLightSource(MakeUniqueName(stage, "Lamp").c_str(), x, y, 0.0f);
				m_selection = { type, static_cast<int>(stage.GetLightSources().size()) - 1 };
				break;
			case ObjectType::LightMirror:
				stage.AddLightMirror(MakeUniqueName(stage, "Mirror").c_str(), x, y, 45.0f, 80.0f);
				m_selection = { type, static_cast<int>(stage.GetLightMirrors().size()) - 1 };
				break;
			case ObjectType::LightReceiver:
				stage.AddLightReceiver(MakeUniqueName(stage, "Receiver").c_str(), x, y, 40.0f, 40.0f);
				m_selection = { type, static_cast<int>(stage.GetLightReceivers().size()) - 1 };
				break;
			case ObjectType::Door:
				stage.AddDoor(MakeUniqueName(stage, "Door").c_str(), x, y, 40.0f, 200.0f, {});
				m_selection = { type, static_cast<int>(stage.GetDoors().size()) - 1 };
				break;
			case ObjectType::LightPlatform:
				stage.AddLightPlatform(MakeUniqueName(stage, "LightPlatform").c_str(), x, y, 150.0f, 25.0f);
				m_selection = { type, static_cast<int>(stage.GetLightPlatforms().size()) - 1 };
				break;
			case ObjectType::FloorSwitch:
				stage.AddFloorSwitch(MakeUniqueName(stage, "Switch").c_str(), x, y, 70.0f, 8.0f);
				m_selection = { type, static_cast<int>(stage.GetFloorSwitches().size()) - 1 };
				break;
			case ObjectType::MistEmitter:
				stage.AddMistEmitter(MakeUniqueName(stage, "Mist").c_str(), x, y, 600.0f, 600.0f, "");
				m_selection = { type, static_cast<int>(stage.GetMistEmitters().size()) - 1 };
				break;
			case ObjectType::MirrorPlatform:
				stage.AddMirrorPlatform(MakeUniqueName(stage, "MirrorStep").c_str(), x, y, 150.0f, 25.0f);
				m_selection = { type, static_cast<int>(stage.GetMirrorPlatforms().size()) - 1 };
				break;
			case ObjectType::Checkpoint:
				stage.AddCheckpoint(MakeUniqueName(stage, "Checkpoint").c_str(), x, y, 80.0f, 200.0f);
				m_selection = { type, static_cast<int>(stage.GetCheckpoints().size()) - 1 };
				break;
			case ObjectType::WarpZone:
				stage.AddWarpZone(MakeUniqueName(stage, "Warp").c_str(), x, y, 150.0f, 200.0f, x - 300.0f, y);
				m_selection = { type, static_cast<int>(stage.GetWarpZones().size()) - 1 };
				break;
			case ObjectType::Indicator:
				stage.AddIndicator(MakeUniqueName(stage, "Lamp").c_str(), x, y, 40.0f, 30.0f, "");
				m_selection = { type, static_cast<int>(stage.GetIndicators().size()) - 1 };
				break;
			case ObjectType::Goal:
				stage.AddGoal(MakeUniqueName(stage, "Goal").c_str(), x, y, 150.0f, 250.0f);
				m_selection = { type, static_cast<int>(stage.GetGoals().size()) - 1 };
				break;
			default:
				break;
			}
		}


		void StageEditor::DuplicateSelected(Stage& stage)
		{
			const int i = m_selection.index;
			switch (m_selection.type)
			{
			case ObjectType::Block:
			{
				StageBlock copy = stage.GetBlocks()[i];
				copy.name = MakeUniqueName(stage, "Block");
				copy.rect.min.x += DUPLICATE_OFFSET; copy.rect.max.x += DUPLICATE_OFFSET;
				stage.GetBlocks().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetBlocks().size()) - 1;
				break;
			}
			case ObjectType::LightSource:
			{
				LightSource copy = stage.GetLightSources()[i];
				copy.name = MakeUniqueName(stage, "Lamp");
				copy.position.x += DUPLICATE_OFFSET;
				stage.GetLightSources().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetLightSources().size()) - 1;
				break;
			}
			case ObjectType::LightMirror:
			{
				LightMirror copy = stage.GetLightMirrors()[i];
				copy.name = MakeUniqueName(stage, "Mirror");
				copy.center.x += DUPLICATE_OFFSET;
				stage.GetLightMirrors().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetLightMirrors().size()) - 1;
				break;
			}
			case ObjectType::LightReceiver:
			{
				LightReceiver copy = stage.GetLightReceivers()[i];
				copy.name = MakeUniqueName(stage, "Receiver");
				copy.rect.min.x += DUPLICATE_OFFSET; copy.rect.max.x += DUPLICATE_OFFSET;
				stage.GetLightReceivers().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetLightReceivers().size()) - 1;
				break;
			}
			case ObjectType::Door:
			{
				Door copy = stage.GetDoors()[i];
				copy.name = MakeUniqueName(stage, "Door");
				copy.closedRect.min.x += DUPLICATE_OFFSET; copy.closedRect.max.x += DUPLICATE_OFFSET;
				stage.GetDoors().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetDoors().size()) - 1;
				break;
			}
			case ObjectType::LightPlatform:
			{
				LightPlatform copy = stage.GetLightPlatforms()[i];
				copy.name = MakeUniqueName(stage, "LightPlatform");
				copy.rect.min.x += DUPLICATE_OFFSET; copy.rect.max.x += DUPLICATE_OFFSET;
				stage.GetLightPlatforms().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetLightPlatforms().size()) - 1;
				break;
			}
			case ObjectType::FloorSwitch:
			{
				FloorSwitch copy = stage.GetFloorSwitches()[i];
				copy.name = MakeUniqueName(stage, "Switch");
				copy.rect.min.x += DUPLICATE_OFFSET; copy.rect.max.x += DUPLICATE_OFFSET;
				stage.GetFloorSwitches().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetFloorSwitches().size()) - 1;
				break;
			}
			case ObjectType::MistEmitter:
			{
				MistEmitter copy = stage.GetMistEmitters()[i];
				copy.name = MakeUniqueName(stage, "Mist");
				copy.area.min.x += DUPLICATE_OFFSET; copy.area.max.x += DUPLICATE_OFFSET;
				stage.GetMistEmitters().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetMistEmitters().size()) - 1;
				break;
			}
			case ObjectType::MirrorPlatform:
			{
				MirrorPlatform copy = stage.GetMirrorPlatforms()[i];
				copy.name = MakeUniqueName(stage, "MirrorStep");
				copy.rect.min.x += DUPLICATE_OFFSET; copy.rect.max.x += DUPLICATE_OFFSET;
				stage.GetMirrorPlatforms().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetMirrorPlatforms().size()) - 1;
				break;
			}
			case ObjectType::Checkpoint:
			{
				Checkpoint copy = stage.GetCheckpoints()[i];
				copy.name = MakeUniqueName(stage, "Checkpoint");
				copy.rect.min.x += DUPLICATE_OFFSET; copy.rect.max.x += DUPLICATE_OFFSET;
				stage.GetCheckpoints().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetCheckpoints().size()) - 1;
				break;
			}
			case ObjectType::WarpZone:
			{
				WarpZone copy = stage.GetWarpZones()[i];
				copy.name = MakeUniqueName(stage, "Warp");
				copy.rect.min.y += DUPLICATE_OFFSET; copy.rect.max.y += DUPLICATE_OFFSET;
				stage.GetWarpZones().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetWarpZones().size()) - 1;
				break;
			}
			case ObjectType::Indicator:
			{
				Indicator copy = stage.GetIndicators()[i];
				copy.name = MakeUniqueName(stage, "Lamp");
				copy.rect.min.y += DUPLICATE_OFFSET; copy.rect.max.y += DUPLICATE_OFFSET;
				stage.GetIndicators().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetIndicators().size()) - 1;
				break;
			}
			case ObjectType::Goal:
			{
				Goal copy = stage.GetGoals()[i];
				copy.name = MakeUniqueName(stage, "Goal");
				copy.rect.min.x += DUPLICATE_OFFSET; copy.rect.max.x += DUPLICATE_OFFSET;
				stage.GetGoals().push_back(copy);
				m_selection.index = static_cast<int>(stage.GetGoals().size()) - 1;
				break;
			}
			default:
				break;
			}
		}


		void StageEditor::DeleteSelected(Stage& stage)
		{
			const int i = m_selection.index;
			switch (m_selection.type)
			{
			case ObjectType::Block:			stage.GetBlocks().erase(stage.GetBlocks().begin() + i); break;
			case ObjectType::LightSource:	stage.GetLightSources().erase(stage.GetLightSources().begin() + i); break;
			case ObjectType::LightMirror:	stage.GetLightMirrors().erase(stage.GetLightMirrors().begin() + i); break;
			case ObjectType::Door:			stage.GetDoors().erase(stage.GetDoors().begin() + i); break;
			case ObjectType::LightPlatform:	stage.GetLightPlatforms().erase(stage.GetLightPlatforms().begin() + i); break;
			case ObjectType::MistEmitter:		stage.GetMistEmitters().erase(stage.GetMistEmitters().begin() + i); break;
			case ObjectType::MirrorPlatform:	stage.GetMirrorPlatforms().erase(stage.GetMirrorPlatforms().begin() + i); break;
			case ObjectType::Checkpoint:		stage.GetCheckpoints().erase(stage.GetCheckpoints().begin() + i); break;
			case ObjectType::WarpZone:		stage.GetWarpZones().erase(stage.GetWarpZones().begin() + i); break;
			case ObjectType::Indicator:		stage.GetIndicators().erase(stage.GetIndicators().begin() + i); break;
			case ObjectType::Goal:			stage.GetGoals().erase(stage.GetGoals().begin() + i); break;
			case ObjectType::LightReceiver:
			{
				// 扉と霧のつながりからも外す
				const std::string name = stage.GetLightReceivers()[i].name;
				stage.GetLightReceivers().erase(stage.GetLightReceivers().begin() + i);
				RemoveSignal(stage, name);
				break;
			}
			case ObjectType::FloorSwitch:
			{
				const std::string name = stage.GetFloorSwitches()[i].name;
				stage.GetFloorSwitches().erase(stage.GetFloorSwitches().begin() + i);
				RemoveSignal(stage, name);
				break;
			}
			default:
				return;
			}
			m_selection = Selection();
			m_isDraggingObject = false;
		}


		bool StageEditor::IsSelectionValid(const Stage& stage) const
		{
			const int i = m_selection.index;
			switch (m_selection.type)
			{
			case ObjectType::None:			return true;
			case ObjectType::BackMirror:	return true;
			case ObjectType::Block:			return i >= 0 && i < static_cast<int>(stage.GetBlocks().size());
			case ObjectType::LightSource:	return i >= 0 && i < static_cast<int>(stage.GetLightSources().size());
			case ObjectType::LightMirror:	return i >= 0 && i < static_cast<int>(stage.GetLightMirrors().size());
			case ObjectType::LightReceiver:	return i >= 0 && i < static_cast<int>(stage.GetLightReceivers().size());
			case ObjectType::Door:			return i >= 0 && i < static_cast<int>(stage.GetDoors().size());
			case ObjectType::LightPlatform:	return i >= 0 && i < static_cast<int>(stage.GetLightPlatforms().size());
			case ObjectType::FloorSwitch:	return i >= 0 && i < static_cast<int>(stage.GetFloorSwitches().size());
			case ObjectType::MistEmitter:	return i >= 0 && i < static_cast<int>(stage.GetMistEmitters().size());
			case ObjectType::MirrorPlatform:return i >= 0 && i < static_cast<int>(stage.GetMirrorPlatforms().size());
			case ObjectType::Checkpoint:	return i >= 0 && i < static_cast<int>(stage.GetCheckpoints().size());
			case ObjectType::WarpZone:		return i >= 0 && i < static_cast<int>(stage.GetWarpZones().size());
			case ObjectType::Indicator:		return i >= 0 && i < static_cast<int>(stage.GetIndicators().size());
			case ObjectType::Goal:			return i >= 0 && i < static_cast<int>(stage.GetGoals().size());
			}
			return false;
		}


		StageEditor::Selection StageEditor::HitTest(const Stage& stage, const math::Vec2& worldPos) const
		{
			const float pickDistance = PICK_DISTANCE_PX / m_zoom;

			// 小さい物・上に描いている物から順に調べる
			for (size_t i = 0; i < stage.GetLightSources().size(); ++i)
			{
				if ((stage.GetLightSources()[i].position - worldPos).Length() <= SOURCE_RADIUS_PX / m_zoom + pickDistance)
				{
					return { ObjectType::LightSource, static_cast<int>(i) };
				}
			}
			for (size_t i = 0; i < stage.GetLightMirrors().size(); ++i)
			{
				const LightMirror& mirror = stage.GetLightMirrors()[i];
				if (DistanceToSegment(worldPos, mirror.GetStart(), mirror.GetEnd()) <= pickDistance)
				{
					return { ObjectType::LightMirror, static_cast<int>(i) };
				}
			}
			// スイッチは薄いので、少し広めに拾う
			for (size_t i = 0; i < stage.GetFloorSwitches().size(); ++i)
			{
				math::Rect rect = stage.GetFloorSwitches()[i].rect;
				rect.min.y -= pickDistance;
				rect.max.y += pickDistance;
				if (Contains(rect, worldPos)) { return { ObjectType::FloorSwitch, static_cast<int>(i) }; }
			}
			for (size_t i = 0; i < stage.GetIndicators().size(); ++i)
			{
				if (Contains(stage.GetIndicators()[i].rect, worldPos)) { return { ObjectType::Indicator, static_cast<int>(i) }; }
			}
			for (size_t i = 0; i < stage.GetLightReceivers().size(); ++i)
			{
				if (Contains(stage.GetLightReceivers()[i].rect, worldPos)) { return { ObjectType::LightReceiver, static_cast<int>(i) }; }
			}
			for (size_t i = 0; i < stage.GetDoors().size(); ++i)
			{
				if (Contains(stage.GetDoors()[i].closedRect, worldPos)) { return { ObjectType::Door, static_cast<int>(i) }; }
			}
			for (size_t i = 0; i < stage.GetLightPlatforms().size(); ++i)
			{
				if (Contains(stage.GetLightPlatforms()[i].rect, worldPos)) { return { ObjectType::LightPlatform, static_cast<int>(i) }; }
			}
			for (size_t i = 0; i < stage.GetMirrorPlatforms().size(); ++i)
			{
				if (Contains(stage.GetMirrorPlatforms()[i].rect, worldPos)) { return { ObjectType::MirrorPlatform, static_cast<int>(i) }; }
			}
			for (size_t i = 0; i < stage.GetCheckpoints().size(); ++i)
			{
				if (Contains(stage.GetCheckpoints()[i].rect, worldPos)) { return { ObjectType::Checkpoint, static_cast<int>(i) }; }
			}
			for (size_t i = 0; i < stage.GetWarpZones().size(); ++i)
			{
				if (Contains(stage.GetWarpZones()[i].rect, worldPos)) { return { ObjectType::WarpZone, static_cast<int>(i) }; }
			}
			for (size_t i = 0; i < stage.GetGoals().size(); ++i)
			{
				if (Contains(stage.GetGoals()[i].rect, worldPos)) { return { ObjectType::Goal, static_cast<int>(i) }; }
			}
			// 部屋全体を覆う物（霧の範囲・奥の鏡）は、枠線の近くをクリックしたときだけ選ぶ
			auto isOnFrame = [&](const math::Rect& frame)
				{
					const bool isInsideOuter = worldPos.x >= frame.min.x - pickDistance && worldPos.x <= frame.max.x + pickDistance
						&& worldPos.y >= frame.min.y - pickDistance && worldPos.y <= frame.max.y + pickDistance;
					const bool isInsideInner = worldPos.x > frame.min.x + pickDistance && worldPos.x < frame.max.x - pickDistance
						&& worldPos.y > frame.min.y + pickDistance && worldPos.y < frame.max.y - pickDistance;
					return isInsideOuter && !isInsideInner;
				};
			for (size_t i = 0; i < stage.GetMistEmitters().size(); ++i)
			{
				if (isOnFrame(stage.GetMistEmitters()[i].area)) { return { ObjectType::MistEmitter, static_cast<int>(i) }; }
			}
			const BackMirror& backMirror = stage.GetBackMirror();
			if (backMirror.isEnabled && m_isShowBackMirror && isOnFrame(backMirror.frame))
			{
				return { ObjectType::BackMirror, 0 };
			}
			// ブロックは後から追加した（上に描かれた）物を優先
			for (size_t i = stage.GetBlocks().size(); i > 0; --i)
			{
				if (Contains(stage.GetBlocks()[i - 1].rect, worldPos)) { return { ObjectType::Block, static_cast<int>(i - 1) }; }
			}
			return Selection();
		}

		StageEditor::GizmoAxis StageEditor::HitTestGizmo(const Stage& stage, const ImVec2& mousePos) const
		{
			if (m_selection.type == ObjectType::None)
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
			const int i = selection.index;
			switch (selection.type)
			{
			case ObjectType::Block:			return stage.GetBlocks()[i].rect.min;
			case ObjectType::LightSource:	return stage.GetLightSources()[i].position;
			case ObjectType::LightMirror:	return stage.GetLightMirrors()[i].center;
			case ObjectType::LightReceiver:	return stage.GetLightReceivers()[i].rect.min;
			case ObjectType::Door:			return stage.GetDoors()[i].closedRect.min;
			case ObjectType::LightPlatform:	return stage.GetLightPlatforms()[i].rect.min;
			case ObjectType::BackMirror:	return stage.GetBackMirror().frame.min;
			case ObjectType::FloorSwitch:	return stage.GetFloorSwitches()[i].rect.min;
			case ObjectType::MistEmitter:	return stage.GetMistEmitters()[i].area.min;
			case ObjectType::MirrorPlatform:return stage.GetMirrorPlatforms()[i].rect.min;
			case ObjectType::Checkpoint:	return stage.GetCheckpoints()[i].rect.min;
			case ObjectType::WarpZone:		return stage.GetWarpZones()[i].rect.min;
			case ObjectType::Indicator:		return stage.GetIndicators()[i].rect.min;
			case ObjectType::Goal:			return stage.GetGoals()[i].rect.min;
			default:						return math::Vec2::Zero();
			}
		}


		void StageEditor::SetAnchor(Stage& stage, const Selection& selection, const math::Vec2& anchor)
		{
			// 矩形は大きさを保ったまま左下を動かす
			auto moveRect = [&anchor](math::Rect& rect)
				{
					const math::Vec2 size{ rect.Width(), rect.Height() };
					rect.min = anchor;
					rect.max = anchor + size;
				};

			const int i = selection.index;
			switch (selection.type)
			{
			case ObjectType::Block:			moveRect(stage.GetBlocks()[i].rect); break;
			case ObjectType::LightSource:	stage.GetLightSources()[i].position = anchor; break;
			case ObjectType::LightMirror:
			{
				// レールに乗っている鏡はレールごと動かす
				LightMirror& mirror = stage.GetLightMirrors()[i];
				const float dx = anchor.x - mirror.center.x;
				mirror.center = anchor;
				mirror.railMinX += dx;
				mirror.railMaxX += dx;
				break;
			}
			case ObjectType::LightReceiver:	moveRect(stage.GetLightReceivers()[i].rect); break;
			case ObjectType::Door:			moveRect(stage.GetDoors()[i].closedRect); break;
			case ObjectType::LightPlatform:	moveRect(stage.GetLightPlatforms()[i].rect); break;
			case ObjectType::BackMirror:	moveRect(stage.GetBackMirror().frame); break;
			case ObjectType::FloorSwitch:	moveRect(stage.GetFloorSwitches()[i].rect); break;
			case ObjectType::MistEmitter:	moveRect(stage.GetMistEmitters()[i].area); break;
			case ObjectType::MirrorPlatform:moveRect(stage.GetMirrorPlatforms()[i].rect); break;
			case ObjectType::Checkpoint:	moveRect(stage.GetCheckpoints()[i].rect); break;
			case ObjectType::WarpZone:		moveRect(stage.GetWarpZones()[i].rect); break;
			case ObjectType::Indicator:		moveRect(stage.GetIndicators()[i].rect); break;
			case ObjectType::Goal:			moveRect(stage.GetGoals()[i].rect); break;
			default: break;
			}
		}


		std::string StageEditor::MakeUniqueName(const Stage& stage, const char* prefix) const
		{
			auto isUsed = [&stage](const std::string& name)
				{
					for (const auto& o : stage.GetBlocks()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetLightSources()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetLightMirrors()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetLightReceivers()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetDoors()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetLightPlatforms()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetFloorSwitches()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetMistEmitters()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetMirrorPlatforms()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetCheckpoints()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetWarpZones()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetIndicators()) { if (o.name == name) return true; }
					for (const auto& o : stage.GetGoals()) { if (o.name == name) return true; }
					return false;
				};
			for (int n = 1; ; ++n)
			{
				const std::string name = std::string(prefix) + std::to_string(n);
				if (!isUsed(name))
				{
					return name;
				}
			}
		}


		void StageEditor::RenameSignal(Stage& stage, const std::string& oldName, const std::string& newName)
		{
			for (Door& door : stage.GetDoors())
			{
				for (std::string& required : door.requiredReceivers)
				{
					if (required == oldName) { required = newName; }
				}
			}
			for (MistEmitter& mist : stage.GetMistEmitters())
			{
				if (mist.triggerSignal == oldName) { mist.triggerSignal = newName; }
			}
			for (Indicator& indicator : stage.GetIndicators())
			{
				if (indicator.signal == oldName) { indicator.signal = newName; }
			}
			for (WarpZone& zone : stage.GetWarpZones())
			{
				if (zone.disabledBySignal == oldName) { zone.disabledBySignal = newName; }
			}
		}


		void StageEditor::RemoveSignal(Stage& stage, const std::string& name)
		{
			// 同じ名前の信号がまだ残っていれば（受光器とスイッチで同じ名前など）つながりは残す
			if (stage.FindReceiverIndex(name) >= 0)
			{
				return;
			}
			for (const FloorSwitch& floorSwitch : stage.GetFloorSwitches())
			{
				if (floorSwitch.name == name) { return; }
			}
			for (Door& door : stage.GetDoors())
			{
				auto& required = door.requiredReceivers;
				required.erase(std::remove(required.begin(), required.end(), name), required.end());
			}
			for (MistEmitter& mist : stage.GetMistEmitters())
			{
				if (mist.triggerSignal == name) { mist.triggerSignal.clear(); }
			}
			for (Indicator& indicator : stage.GetIndicators())
			{
				if (indicator.signal == name) { indicator.signal.clear(); }
			}
			for (WarpZone& zone : stage.GetWarpZones())
			{
				if (zone.disabledBySignal == name) { zone.disabledBySignal.clear(); }
			}
		}


		void StageEditor::SignalCombo(const Stage& stage, const char* label, std::string& signal)
		{
			if (ImGui::BeginCombo(label, signal.empty() ? "(none)" : signal.c_str()))
			{
				if (ImGui::Selectable("(none)", signal.empty())) { signal.clear(); }
				for (const LightReceiver& receiver : stage.GetLightReceivers())
				{
					if (ImGui::Selectable(receiver.name.c_str(), signal == receiver.name)) { signal = receiver.name; }
				}
				for (const FloorSwitch& floorSwitch : stage.GetFloorSwitches())
				{
					if (ImGui::Selectable(floorSwitch.name.c_str(), signal == floorSwitch.name)) { signal = floorSwitch.name; }
				}
				ImGui::EndCombo();
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
			, m_exportText()
			, m_isShowExport(false)
			, m_functionName("BuildStage01")
		{}
	}
}
