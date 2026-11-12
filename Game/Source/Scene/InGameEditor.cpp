/**
 * @file InGameEditor.cpp
 * @brief インゲームのデバッグウィンドウ「Game (F2)」
 */
#include "stdafx.h"
#include "InGameEditor.h"

#include "imgui.h"
#include "MistEffectParameter.h"
#include "Source/Actor/Character/Player/Player.h"
#include "Source/Camera/GameCamera.h"
#include "Source/Render/MirrorGlass.h"
#include "Source/Sound/GameplaySEPlayer.h"
#include "Source/Stage/Editor/StageDebugOverlay.h"
#include "Source/Stage/GimmickMarkRenderer.h"
#include "Source/Stage/GimmickRenderer.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/StageCatalog.h"
#include "Source/Stage/StageRenderer.h"
#include "Source/Timer/StageTimer.h"
#include "Source/UI/MirrorPromptUI.h"
#include "Source/UI/TimerUI.h"


namespace
{
	/** ウィンドウを最初に出す位置と幅 */
	const ImVec2 WINDOW_POSITION(20.0f, 20.0f);
	const ImVec2 WINDOW_SIZE(340.0f, 0.0f);
	/** 読み込みのメッセージの色 */
	const ImVec4 MESSAGE_COLOR(1.0f, 0.5f, 0.4f, 1.0f);
}


namespace app
{
	void InGameEditor::Init(const InGameEditTargets& targets)
	{
		m_targets = targets;
	}


	InGameEditorRequest InGameEditor::Draw(const InGameStatus& status)
	{
		if (!m_isOpen || !IsReady())
		{
			return InGameEditorRequest();
		}

		ImGui::SetNextWindowPos(WINDOW_POSITION, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(WINDOW_SIZE, ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Game (F2)", &m_isOpen))
		{
			ImGui::End();
			return InGameEditorRequest();
		}

		DrawHelp();
		const bool isRestart = DrawPlayer();
		InGameEditorRequest request = DrawStage(status);
		request.isRestart = request.isRestart || isRestart;
		DrawFeatureEditors();
		DrawDisplay(status);

		ImGui::End();
		return request;
	}


	void InGameEditor::DrawHelp()
	{
		ImGui::TextUnformatted("F1: Stage Editor   F2: This window   F3: Whole Stage View   F5: Restart");
		ImGui::TextUnformatted("Mouse: drag the player (screen edge = scroll) / Ctrl+Click: warp there");
	}


	bool InGameEditor::DrawPlayer()
	{
		ImGui::SeparatorText("Player");
		m_playerEditor.DrawStatus(*m_targets.player);

		const bool isRestart = ImGui::Button("Restart (F5)");
		ImGui::SetItemTooltip("Keep the stage layout. Reset players, doors, lights and mist. Respawn at the last checkpoint.");
		return isRestart;
	}


	InGameEditorRequest InGameEditor::DrawStage(const InGameStatus& status)
	{
		InGameEditorRequest request;

		ImGui::SeparatorText("Stage");
		const int currentStage = stage::GetCurrentStageIndex();
		ImGui::Text("%s  (%d / %d)", stage::STAGE_INFO_TABLE[currentStage].name, currentStage + 1, stage::STAGE_COUNT);
		// ステージの切り替え（StageCatalog.h の表から選ぶ）
		if (ImGui::BeginCombo("Stage", stage::STAGE_INFO_TABLE[currentStage].name))
		{
			for (int i = 0; i < stage::STAGE_COUNT; ++i)
			{
				if (ImGui::Selectable(stage::STAGE_INFO_TABLE[i].name, i == currentStage) && i != currentStage)
				{
					request.changeStageIndex = i;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SetItemTooltip("Load another stage from its JSON files. Unsaved editor changes are lost.");

		// ゴールに触れたのと同じ扱いにする（次のステージへ進む流れ・ゲームクリアの確認用）
		ImGui::BeginDisabled(status.isCleared);
		request.isClearStage = ImGui::Button("Clear This Stage");
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Same as touching the goal: next stage, or the result screen after the last stage.");

		request.isReloadStage = ImGui::Button("Reload JSON");
		ImGui::SetItemTooltip("Rebuild the stage from the JSON files (%s). Unsaved editor changes are lost.", stage::GetCurrentStageInfo().name);

		// 読み込みのメッセージ（失敗の理由・飛ばしたギミック）
		if (status.stageMessages != nullptr)
		{
			for (const std::string& message : *status.stageMessages)
			{
				ImGui::TextColored(MESSAGE_COLOR, "%s", message.c_str());
			}
		}
		ImGui::Checkbox("Auto restart on death", &m_isAutoRestart);
		ImGui::Text("Respawn: %s (%.0f, %.0f)", status.respawn.name.empty() ? "Start" : status.respawn.name.c_str(),
			status.respawn.position.x, status.respawn.position.y);
		ImGui::Text("Warps: %d   %s", status.warpCount, status.isCleared ? "CLEARED!" : "");

		// チェックポイントへワープ（部屋2などを最初から確かめたいとき）
		const std::optional<RespawnPoint> warpTo = DrawWarpButtons(status);
		if (warpTo.has_value())
		{
			request.isWarp = true;
			request.warpTo = warpTo.value();
		}
		return request;
	}


	std::optional<RespawnPoint> InGameEditor::DrawWarpButtons(const InGameStatus& status)
	{
		const auto checkpoints = m_targets.stage->GetGimmicks().View<stage::Checkpoint>();
		if (checkpoints.empty())
		{
			return std::nullopt;
		}

		std::optional<RespawnPoint> warpTo;
		ImGui::TextUnformatted("Warp:");
		if (ImGui::SmallButton("Start"))
		{
			// 名前が空 = ステージの開始位置
			warpTo = RespawnPoint{ std::string(), status.startPosition };
		}
		for (const stage::Checkpoint& checkpoint : checkpoints)
		{
			ImGui::SameLine();
			ImGui::PushID(&checkpoint);
			if (ImGui::SmallButton(checkpoint.name.empty() ? "(no name)" : checkpoint.name.c_str()))
			{
				warpTo = RespawnPoint{ checkpoint.name, checkpoint.GetSpawnPosition() };
			}
			ImGui::PopID();
		}
		return warpTo;
	}


	void InGameEditor::DrawFeatureEditors()
	{
		// ---- 時間制限（中身はタイマーと表示のクラスが作る） ----
		if (ImGui::CollapsingHeader("Timer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			m_targets.stageTimer->DrawSettingsUI();
			m_targets.timerUI->DrawSettingsUI();
		}
		// ---- 反射鏡のインタラクト表示 ----
		if (ImGui::CollapsingHeader("Mirror Prompt"))
		{
			m_targets.mirrorPromptUI->DrawSettingsUI();
		}
		// ---- ギミックの印（電球マーク・歯車・レール） ----
		if (ImGui::CollapsingHeader("Gimmick Marks"))
		{
			m_gimmickMarkEditor.Draw(*m_targets.gimmickMarkRenderer);
		}
		// ---- 動きの調整 ----
		if (ImGui::CollapsingHeader("Tuning"))
		{
			m_playerEditor.DrawTuning(m_targets.player->GetPlayerParameterForEdit());
		}
		// ---- アウトライン（黒い縁） ----
		if (ImGui::CollapsingHeader("Outline"))
		{
			m_outlineEditor.Draw(*RenderingEngine::GetInstance());
		}
		// ---- ブルーム（明るい所から光があふれる効果） ----
		if (ImGui::CollapsingHeader("Bloom"))
		{
			m_bloomEditor.Draw(*RenderingEngine::GetInstance());
		}
		// ---- カメラ ----
		if (ImGui::CollapsingHeader("Camera"))
		{
			m_cameraEditor.Draw(*m_targets.camera);
		}
		// ---- SE ----
		if (ImGui::CollapsingHeader("Sound"))
		{
			m_seEditor.Draw(*m_targets.sePlayer);
		}
	}


	void InGameEditor::DrawDisplay(const InGameStatus& status)
	{
		if (!ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}
		m_targets.overlay->DrawSettingsUI();
		if (ImGui::TreeNode("Mist Effect"))
		{
			DrawMistEffect(status);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Mirror Glass"))
		{
			m_mirrorGlassEditor.Draw(*m_targets.mirrorGlass);
			ImGui::TreePop();
		}
		DrawStageView();
	}


	void InGameEditor::DrawMistEffect(const InGameStatus& status)
	{
		MistEffectParameter& mist = *m_targets.mistEffect;
		ImGui::Checkbox("Enabled", &mist.isEnabled);
		ImGui::DragFloat("Puff Scale", &mist.puffScale, 0.5f, 1.0f, 500.0f, "%.1f");
		ImGui::DragFloat("Puff Interval", &mist.puffInterval, 0.01f, 0.02f, 2.0f, "%.2f s");
		ImGui::DragFloat("Puff Band", &mist.puffBand, 5.0f, 0.0f, 2000.0f, "%.0f");
		ImGui::SetItemTooltip("Puffs appear between the mist front and this height above it.");
		ImGui::DragFloat("Puff Depth", &mist.puffDepth, 5.0f, 0.0f, 1000.0f, "%.0f");
		ImGui::SliderFloat("Fill Rate", &mist.fillRate, 0.0f, 1.0f, "%.2f");
		ImGui::SetItemTooltip("Chance of an extra puff above the mist front (so the mist stays in the room).");
		ImGui::DragFloat("Reveal Puff Rate", &mist.dewPuffScaleRate, 0.01f, 0.0f, 3.0f, "%.2f");
		ImGui::Text("Puffs played: %d", status.mistPuffCount);
	}


	void InGameEditor::DrawStageView()
	{
		stage::StageRenderer::Config& stageConfig = m_targets.stageRenderer->GetConfig();
		ImGui::DragFloat("Floor Depth", &stageConfig.depth, 5.0f, 10.0f, 5000.0f, "%.0f");
		ImGui::DragFloat("Stone Texture Size", &stageConfig.stoneTextureWorldSize, 5.0f, 25.0f, 2000.0f, "%.0f");
		ImGui::SetItemTooltip("World length of one rock.DDS tile on the stage blocks. Smaller = smaller stones.");
		ImGui::Checkbox("Gimmick Reflection", &m_targets.gimmickRenderer->GetConfig().isDrawReflection);
		ImGui::SetItemTooltip("Keep the Back Mirror's Plane Z at half of this so the mirror sits at the back edge of the floor.");
		stage::BackMirror& backMirror = m_targets.stage->GetBackMirror();
		ImGui::Checkbox("Back Mirror", &backMirror.isEnabled);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat("Plane Z", &backMirror.planeZ, 1.0f, -5000.0f, 5000.0f, "%.0f");
	}


	bool InGameEditor::IsReady() const
	{
		const InGameEditTargets& t = m_targets;
		return t.player != nullptr && t.stage != nullptr && t.stageTimer != nullptr && t.timerUI != nullptr
			&& t.mirrorPromptUI != nullptr && t.camera != nullptr && t.sePlayer != nullptr && t.overlay != nullptr
			&& t.mirrorGlass != nullptr && t.stageRenderer != nullptr && t.gimmickRenderer != nullptr
			&& t.gimmickMarkRenderer != nullptr && t.mistEffect != nullptr;
	}
}
