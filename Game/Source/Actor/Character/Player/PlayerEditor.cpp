/**
 * @file PlayerEditor.cpp
 * @brief プレイヤーの状態の表示と、動きの調整値を変える ImGui
 */
#include "stdafx.h"
#include "PlayerEditor.h"

#include "Player.h"
#include "Source/Actor/Character/CharacterStateMachine.h"
#include "Source/Actor/Character/CharacterStatus.h"
#include "imgui.h"


namespace app
{
	namespace actor
	{
		void PlayerEditor::DrawStatus(const Player& player)
		{
			const CharacterController& controller = player.GetController();
			ImGui::Text("State : %s", player.GetStateMachine()->GetCurrentStateName());
			ImGui::Text("HP    : %d / %d", player.GetStatus()->GetHp(), player.GetStatus()->GetMaxHp());
			ImGui::Text("Pos   : (%.1f, %.1f)", player.GetPosition().x, player.GetPosition().y);
			ImGui::Text("Vel   : (%.1f, %.1f)", controller.GetVelocity().x, controller.GetVelocity().y);
			ImGui::Text("Ground: %s  Wall L/R: %s/%s", controller.IsOnGround() ? "yes" : "no",
				controller.IsTouchingWall(-1) ? "yes" : "no", controller.IsTouchingWall(1) ? "yes" : "no");
		}


		void PlayerEditor::DrawTuning(PlayerParameter& param)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("PlayerEditor");
			DrawMovement(param);
			ImGui::Separator();
			DrawModelAndBody(param);
			ImGui::PopID();
		}


		void PlayerEditor::DrawMovement(PlayerParameter& param)
		{
			ImGui::DragFloat("Run Speed", &param.runSpeed, 5.0f, 0.0f, 3000.0f, "%.0f");
			ImGui::DragFloat("Run Anim Speed", &param.runAnimationSpeed, 0.01f, 0.1f, 5.0f, "x%.2f");
			ImGui::SetItemTooltip("Playback speed of the run animation.\nFeet match when Run Speed is about 470 x this value.");
			ImGui::DragFloat("Jump Velocity", &param.jumpVelocity, 10.0f, 0.0f, 5000.0f, "%.0f");
			ImGui::DragFloat("Gravity", &param.gravity, 10.0f, -20000.0f, 0.0f, "%.0f");
			ImGui::DragFloat("Max Fall Speed", &param.maxFallSpeed, 10.0f, 0.0f, 10000.0f, "%.0f");
			ImGui::DragFloat("Ledge Reach", &param.ledgeGrabReach, 1.0f, 0.0f, 200.0f, "%.0f");
			ImGui::DragFloat("Ledge Above", &param.ledgeGrabAbove, 1.0f, 0.0f, 200.0f, "%.0f");
			ImGui::DragFloat("Climb Duration", &param.climbUpDuration, 0.01f, 0.05f, 5.0f, "%.2f s");
			ImGui::SetItemTooltip("Length of the climb animation at normal speed.");
			ImGui::DragFloat("Climb Anim Speed", &param.climbAnimationSpeed, 0.01f, 0.1f, 5.0f, "x%.2f");
			ImGui::SetItemTooltip("Playback speed of the climb animation.\nClimb time = Climb Duration / this value.");
			ImGui::DragFloat("Grab Move Speed", &param.grabMoveSpeed, 5.0f, 0.0f, 3000.0f, "%.0f");
			ImGui::SetItemTooltip("Move speed while dragging a mirror / shade board.");
			ImGui::DragFloat("Rotate Action Time", &param.rotateActionTime, 0.01f, 0.0f, 3.0f, "%.2f s");
			ImGui::SetItemTooltip("How long the grab-idle animation plays after rotating a mirror.");
			ImGui::DragFloat("Hang Model Forward", &param.ledgeHangModelForward, 0.5f, -200.0f, 200.0f, "%.1f");
			ImGui::DragFloat("Hang Model Y", &param.ledgeHangModelY, 0.5f, -200.0f, 200.0f, "%.1f");
			ImGui::SetItemTooltip("Shift the model while hanging so the hands are on the ledge corner.");
			ImGui::DragFloat("Climb Model Forward", &param.climbModelForward, 0.5f, -200.0f, 200.0f, "%.1f");
			ImGui::DragFloat("Climb Model Y", &param.climbModelY, 0.5f, -200.0f, 200.0f, "%.1f");
			ImGui::SetItemTooltip("Added to the hang shift while climbing. Match the first frame of the climb to the hang pose.");
			ImGui::DragFloat("Hard Landing Height", &param.hardLandingHeight, 5.0f, 0.0f, 3000.0f, "%.0f");
			ImGui::DragFloat("Deadly Fall Height", &param.deadlyFallHeight, 5.0f, 0.0f, 5000.0f, "%.0f");
			ImGui::Checkbox("Auto Ledge Grab", &param.isAutoLedgeGrab);
		}


		void PlayerEditor::DrawModelAndBody(PlayerParameter& param)
		{
			ImGui::DragFloat("Model Yaw Offset", &param.modelYawOffsetDeg, 1.0f, -180.0f, 180.0f, "%.0f deg");
			ImGui::SetItemTooltip("Set 180 if the model faces backwards.");
			ImGui::DragFloat("Model Scale", &param.modelScale, 0.01f, 0.05f, 5.0f, "%.2f");
			ImGui::SetItemTooltip("Model size only. Collision size is Body Width / Body Height.");
			ImGui::DragFloat("Body Width", &param.bodyWidth, 1.0f, 10.0f, 300.0f, "%.0f");
			ImGui::DragFloat("Body Height", &param.bodyHeight, 1.0f, 10.0f, 400.0f, "%.0f");
			ImGui::DragFloat("Crouch Height", &param.crouchHeight, 1.0f, 10.0f, 400.0f, "%.0f");
			ImGui::TextDisabled("Body size is applied on Restart.");
		}
	}
}
