/**
 * @file CameraEditor.cpp
 * @brief ゲームカメラを調整する ImGui
 */
#include "stdafx.h"
#include "CameraEditor.h"

#include "GameCamera.h"
#include "imgui.h"


namespace
{
	/** 見え方の番号（ラジオボタン用） */
	constexpr int VIEW_MODE_FOLLOW = 0;
	constexpr int VIEW_MODE_OVERVIEW = 1;
	/** 注意の文字の色 */
	const ImVec4 WARNING_COLOR(1.0f, 0.6f, 0.2f, 1.0f);
}


namespace app
{
	namespace camera
	{
		void CameraEditor::Draw(GameCamera& camera)
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("CameraEditor");

			DrawViewMode(camera);
			DrawProjection(camera);
			DrawFollow(camera);
			DrawClipAndStatus(camera);

			ImGui::PopID();
		}


		void CameraEditor::DrawViewMode(GameCamera& camera)
		{
			CameraParameter& param = camera.GetParameterForEdit();

			ImGui::SeparatorText("View");
			int mode = param.isOverview ? VIEW_MODE_OVERVIEW : VIEW_MODE_FOLLOW;
			ImGui::RadioButton("Follow", &mode, VIEW_MODE_FOLLOW);
			ImGui::SetItemTooltip("Follow the player (normal view).");
			ImGui::SameLine();
			ImGui::RadioButton("Whole Stage", &mode, VIEW_MODE_OVERVIEW);
			ImGui::SetItemTooltip("Show the whole stage from its center. (F3 also toggles this)");
			param.isOverview = (mode == VIEW_MODE_OVERVIEW);

			if (!param.isOverview)
			{
				return;
			}

			// 全体表示の調整
			ImGui::DragFloat("Margin", &param.overviewMargin, 10.0f, 0.0f, 5000.0f, "%.0f");
			ImGui::SetItemTooltip("Space around the stage (each side).");
			if (!camera.HasStageBounds())
			{
				ImGui::TextColored(WARNING_COLOR, "No stage blocks - following the player.");
				return;
			}
			const math::Rect& bounds = camera.GetStageBounds();
			ImGui::Text("Stage: %.0f x %.0f", bounds.Width(), bounds.Height());
			ImGui::Text("Center: (%.0f, %.0f)", (bounds.min.x + bounds.max.x) * 0.5f, (bounds.min.y + bounds.max.y) * 0.5f);
		}


		void CameraEditor::DrawProjection(GameCamera& camera)
		{
			CameraParameter& param = camera.GetParameterForEdit();

			ImGui::SeparatorText("Projection");
			ImGui::Checkbox("Orthographic", &param.isOrtho);
			if (param.isOrtho)
			{
				if (param.isOverview && camera.HasStageBounds())
				{
					// 全体表示のときは、ステージが入るズームを自動で決める
					ImGui::Text("Zoom: %.2f (auto)", camera.GetCurrentZoom());
				}
				else
				{
					ImGui::DragFloat("Zoom", &param.zoom, 0.01f, limit::MIN_ZOOM, limit::MAX_ZOOM, "%.2f");
					ImGui::SetItemTooltip("Bigger = wider view (zoom out).");
				}
			}
			else
			{
				ImGui::DragFloat("View Angle", &param.viewAngleDeg, 0.5f, limit::MIN_VIEW_ANGLE, limit::MAX_VIEW_ANGLE, "%.0f deg");
			}
		}


		void CameraEditor::DrawFollow(GameCamera& camera)
		{
			CameraParameter& param = camera.GetParameterForEdit();

			ImGui::SeparatorText("Follow");
			if (param.isOverview && !param.isOrtho && camera.HasStageBounds())
			{
				// 透視投影の全体表示は、ステージが入る距離を自動で決める
				ImGui::Text("Distance: %.0f (auto)", camera.GetCurrentDistance());
			}
			else
			{
				ImGui::DragFloat("Distance", &param.followDistance, 10.0f, limit::MIN_DISTANCE, limit::MAX_DISTANCE, "%.0f");
				if (param.isOrtho)
				{
					ImGui::SetItemTooltip("Orthographic: does not change the view size. Use Zoom instead.");
				}
			}
			ImGui::DragFloat("Height", &param.height, 5.0f, -2000.0f, 2000.0f, "%.0f");
			ImGui::DragFloat("Target Offset Y", &param.targetOffsetY, 1.0f, -1000.0f, 1000.0f, "%.0f");
			ImGui::DragFloat("Smoothing", &param.smoothing, 0.1f, 0.1f, 30.0f, "%.1f");
			ImGui::SetItemTooltip("Bigger = catches up faster. Also the speed of switching views.");
		}


		void CameraEditor::DrawClipAndStatus(GameCamera& camera)
		{
			CameraParameter& param = camera.GetParameterForEdit();

			ImGui::SeparatorText("Clip");
			ImGui::DragFloat("Near", &param.nearClip, 1.0f, 0.1f, 1000.0f, "%.1f");
			ImGui::DragFloat("Far", &param.farClip, 100.0f, 100.0f, 100000.0f, "%.0f");
			ImGui::SetItemTooltip("Objects farther than this from the camera are not drawn.");

			ImGui::Separator();
			const Vector3& target = camera.GetCurrentTarget();
			ImGui::Text("Target: (%.0f, %.0f, %.0f)", target.x, target.y, target.z);
			if (param.isOrtho)
			{
				// 今、画面に映っている範囲
				const float viewHeight = camera.GetCurrentViewHeight();
				ImGui::Text("View Size: %.0f x %.0f", viewHeight * camera.GetAspect(), viewHeight);
			}
			if (ImGui::Button("Reset Camera"))
			{
				camera.ResetParameter();
			}
			ImGui::SetItemTooltip("Back to the values given in Init.");
		}
	}
}
