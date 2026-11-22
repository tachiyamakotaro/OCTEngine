/**
 * @file EventCameraEditor.cpp
 * @brief イベントカメラを調整する ImGui
 */
#include "stdafx.h"
#include "EventCameraEditor.h"

#include "CameraParameter.h"
#include "EventCamera.h"
#include "imgui.h"


namespace app
{
	namespace camera
	{
		void EventCameraEditor::Draw(EventCamera& eventCamera)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("EventCameraEditor");

			DrawStatus(eventCamera);
			EventCameraParameter& param = eventCamera.GetParameterForEdit();
			DrawShotSetting(eventCamera, CameraEventKind::GimmickSolved, param.gimmickSolved);
			DrawShotSetting(eventCamera, CameraEventKind::Goal, param.goal);
			DrawShotSetting(eventCamera, CameraEventKind::StageStart, param.stageStart);
			DrawShotSetting(eventCamera, CameraEventKind::Death, param.death);

			ImGui::PopID();
		}


		void EventCameraEditor::DrawStatus(EventCamera& eventCamera)
		{
			ImGui::SeparatorText("Status");
			if (!eventCamera.IsPlaying())
			{
				ImGui::Text("Playing: -");
				return;
			}
			ImGui::Text("Playing: %s", CameraEventKindToString(eventCamera.GetPlayingEvent().GetKind()));
			ImGui::ProgressBar(eventCamera.GetWeight(), ImVec2(-FLT_MIN, 0.0f), "Weight");
			if (ImGui::Button("Stop"))
			{
				eventCamera.Reset();
			}
		}


		void EventCameraEditor::DrawShotSetting(EventCamera& eventCamera, const CameraEventKind kind, EventShotSetting& setting)
		{
			ImGui::SeparatorText(CameraEventKindToString(kind));
			// 同じラベルが2つの場面に出るので、場面ごとに ID を分ける
			ImGui::PushID(static_cast<int>(kind));

			ImGui::Checkbox("Enabled", &setting.isEnabled);
			ImGui::SameLine();
			if (ImGui::Button("Test"))
			{
				eventCamera.Play(CameraEvent::PlayerOnly(kind));
			}
			ImGui::SetItemTooltip("Zoom in on the player only (without solving a gimmick).");

			ImGui::DragFloat("Zoom", &setting.zoom, 0.01f, limit::MIN_ZOOM, limit::MAX_ZOOM, "%.2f");
			ImGui::SetItemTooltip("Smaller = closer. Normal view is Camera > Zoom.\nNever zooms out more than the normal view.");
			ImGui::DragFloat("Hold", &setting.holdTime, 0.05f, 0.0f, 30.0f, "%.2f s");
			ImGui::SetItemTooltip("Stay zoomed for this time.\n(Stage Start: only for Test. In the game, it zooms out when the circle starts to open.)");
			ImGui::DragFloat("Blend In", &setting.blendInTime, 0.01f, 0.0f, 5.0f, "%.2f s");
			ImGui::SetItemTooltip("Time to zoom in. 0 = already zoomed in at the start.");
			ImGui::DragFloat("Blend Out", &setting.blendOutTime, 0.01f, 0.0f, 5.0f, "%.2f s");
			ImGui::SetItemTooltip("Time to go back to the normal view.\n(Stage Start: same as Stage Start > Circle Open = zoom out with the circle.)");
			ImGui::DragFloat("Margin", &setting.margin, 5.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::SetItemTooltip("Space around the player (and the gimmick).");
			ImGui::Checkbox("Include Target", &setting.isIncludeTarget);
			ImGui::SetItemTooltip("Also keep the gimmick / goal on screen.\nIf it is far away, the camera zooms in less.");
			ImGui::Checkbox("Lock Player", &setting.isLockPlayer);
			ImGui::SetItemTooltip("The player cannot move (and the timer stops) while zoomed in.\nThe player can move again when the camera starts to zoom out.");

			ImGui::PopID();
		}
	}
}
