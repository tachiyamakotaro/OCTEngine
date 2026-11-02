/**
 * @file GameplaySEEditor.cpp
 * @brief インゲームの SE（GameplaySEPlayer）を調整する ImGui
 */
#include "stdafx.h"
#include "GameplaySEEditor.h"

#include "GameplaySEPlayer.h"
#include "SoundManager.h"
#include "imgui.h"


namespace
{
	/** 注意の文字の色 */
	const ImVec4 WARNING_COLOR(1.0f, 0.6f, 0.2f, 1.0f);
}


namespace app
{
	void GameplaySEEditor::Draw(GameplaySEPlayer& sePlayer)
	{
		// ほかの調整項目とラベルがかぶっても混ざらないようにする
		ImGui::PushID("GameplaySEEditor");
		DrawTiming(sePlayer);
		DrawLastPlayed(sePlayer);
		ImGui::PopID();
	}


	void GameplaySEEditor::DrawTiming(GameplaySEPlayer& sePlayer)
	{
		ImGui::Checkbox("Gameplay SE", &sePlayer.isEnabled);
		ImGui::DragFloat("Footstep Interval", &sePlayer.footstepInterval, 0.01f, 0.05f, 2.0f, "%.2f s");
		ImGui::DragFloat("First Footstep Delay", &sePlayer.firstFootstepDelay, 0.01f, 0.0f, 1.0f, "%.2f s");
		ImGui::DragFloat("Min Footstep Speed", &sePlayer.minFootstepSpeed, 1.0f, 0.0f, 2000.0f, "%.0f");
		ImGui::DragFloat("Min Air Time (Land)", &sePlayer.minAirTimeForLand, 0.01f, 0.0f, 2.0f, "%.2f s");
		ImGui::SetItemTooltip("No landing sound after a shorter time in the air (stepping down a small ledge).");
		ImGui::DragFloat("Slide Stop Delay", &sePlayer.slideStopDelay, 0.01f, 0.0f, 1.0f, "%.2f s");
	}


	void GameplaySEEditor::DrawLastPlayed(const GameplaySEPlayer& sePlayer)
	{
		const int lastKind = sePlayer.GetLastPlayedKind();
		const char* lastPath = GetSoundAssetPath(lastKind);
		ImGui::Text("Played: %d", sePlayer.GetPlayCount());
		ImGui::Text("Last: %s", lastPath != nullptr ? lastPath : "-");
		if (lastPath != nullptr && !SoundManager::Get().IsLoaded(lastKind))
		{
			ImGui::TextColored(WARNING_COLOR, "(file not found - silent)");
		}
	}
}
