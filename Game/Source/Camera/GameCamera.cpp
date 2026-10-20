/**
 * @file GameCamera.cpp
 * @brief インゲームのカメラ
 */
#include "stdafx.h"
#include "GameCamera.h"

#include "imgui.h"
#include <cmath>


namespace
{
	/** カメラの距離の範囲 */
	constexpr float MIN_DISTANCE = 500.0f;
	constexpr float MAX_DISTANCE = 6000.0f;
	/** 平行投影のズームの範囲 */
	constexpr float MIN_ZOOM = 0.25f;
	constexpr float MAX_ZOOM = 5.0f;
	/** 透視投影の画角の範囲（度） */
	constexpr float MIN_VIEW_ANGLE = 10.0f;
	constexpr float MAX_VIEW_ANGLE = 120.0f;
	/** 近平面と遠平面の最低限の間隔 */
	constexpr float MIN_CLIP_GAP = 1.0f;
	/** 画面サイズが取れなかったときのアスペクト比 */
	constexpr float DEFAULT_ASPECT = 16.0f / 9.0f;

	/** value を minValue〜maxValue の間に収める */
	float ClampFloat(const float value, const float minValue, const float maxValue)
	{
		if (value < minValue)
		{
			return minValue;
		}
		if (value > maxValue)
		{
			return maxValue;
		}
		return value;
	}
}


namespace app
{
	namespace camera
	{
		GameCamera::GameCamera()
			: m_camera(nullptr)
			, m_parameter()
			, m_initialParameter()
			, m_currentTarget(0.0f, 0.0f, 0.0f)
			, m_currentDistance(0.0f)
		{}


		void GameCamera::Init(Camera* camera, const CameraParameter& parameter)
		{
			m_camera = camera;
			m_parameter = parameter;
			m_initialParameter = parameter;
			m_currentDistance = ClampFloat(m_parameter.followDistance, MIN_DISTANCE, MAX_DISTANCE);
			if (m_camera != nullptr)
			{
				ApplyProjection();
			}
		}


		void GameCamera::Update(const float deltaTime, const Vector3& followPosition)
		{
			// なめらかに追いかける（フレームレートが変わっても同じ速さになる書き方）
			const float rate = 1.0f - std::exp(-m_parameter.smoothing * deltaTime);
			Follow(followPosition, rate);
		}


		void GameCamera::SnapTo(const Vector3& followPosition)
		{
			Follow(followPosition, 1.0f);
		}


		void GameCamera::Follow(const Vector3& followPosition, const float rate)
		{
			// Init の前に呼ばれたら何もしない
			if (m_camera == nullptr)
			{
				return;
			}

			const Vector3 desiredTarget(followPosition.x, followPosition.y + m_parameter.targetOffsetY, followPosition.z);
			const float desiredDistance = ClampFloat(m_parameter.followDistance, MIN_DISTANCE, MAX_DISTANCE);

			m_currentTarget.x += (desiredTarget.x - m_currentTarget.x) * rate;
			m_currentTarget.y += (desiredTarget.y - m_currentTarget.y) * rate;
			m_currentTarget.z += (desiredTarget.z - m_currentTarget.z) * rate;
			m_currentDistance += (desiredDistance - m_currentDistance) * rate;

			// ImGui で変えた投影の値や、画面サイズの変化も反映されるよう毎回設定し直す
			ApplyProjection();
			ApplyView();
		}


		void GameCamera::ApplyProjection()
		{
			// 近平面と遠平面（遠平面が近平面より手前にならないようにする）
			const float nearClip = m_parameter.nearClip > 0.0f ? m_parameter.nearClip : CameraParameter().nearClip;
			const float farClip = m_parameter.farClip > nearClip + MIN_CLIP_GAP ? m_parameter.farClip : nearClip + MIN_CLIP_GAP;
			m_camera->SetNear(nearClip);
			m_camera->SetFar(farClip);

			if (m_parameter.isOrtho)
			{
				// 平行投影：縦に映る範囲をズームで決めて、横は画面のアスペクト比に合わせる（横に伸びないように）
				const float frameWidth = static_cast<float>(g_graphicsEngine->GetFrameBufferWidth());
				const float frameHeight = static_cast<float>(g_graphicsEngine->GetFrameBufferHeight());
				const float aspect = frameHeight > 0.0f ? frameWidth / frameHeight : DEFAULT_ASPECT;
				const float viewHeight = m_parameter.baseViewHeight * ClampFloat(m_parameter.zoom, MIN_ZOOM, MAX_ZOOM);

				m_camera->SetUpdateProjMatrixFunc(Camera::enUpdateProjMatrixFunc_Ortho);
				m_camera->SetHeight(viewHeight);
				m_camera->SetWidth(viewHeight * aspect);
			}
			else
			{
				// 透視投影：画角で見える範囲が決まる（距離を離すほど引いた視点になる）
				const float viewAngle = ClampFloat(m_parameter.viewAngleDeg, MIN_VIEW_ANGLE, MAX_VIEW_ANGLE);

				m_camera->SetUpdateProjMatrixFunc(Camera::enUpdateProjMatrixFunc_Perspective);
				m_camera->SetViewAngle(Math::DegToRad(viewAngle));
			}
		}


		void GameCamera::ApplyView()
		{
			// 手前(-Z)から、少し上から見る
			m_camera->SetTarget(m_currentTarget);
			m_camera->SetPosition(Vector3(
				m_currentTarget.x,
				m_currentTarget.y + m_parameter.height,
				m_currentTarget.z - m_currentDistance));
		}


		void GameCamera::DrawSettingsUI()
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("GameCamera");

			// ---- 投影 ----
			ImGui::Checkbox("Orthographic", &m_parameter.isOrtho);
			if (m_parameter.isOrtho)
			{
				ImGui::DragFloat("Zoom", &m_parameter.zoom, 0.01f, MIN_ZOOM, MAX_ZOOM, "%.2f");
				ImGui::SetItemTooltip("Bigger = wider view (zoom out).");
			}
			else
			{
				ImGui::DragFloat("View Angle", &m_parameter.viewAngleDeg, 0.5f, MIN_VIEW_ANGLE, MAX_VIEW_ANGLE, "%.0f deg");
			}

			// ---- 追従 ----
			ImGui::DragFloat("Distance", &m_parameter.followDistance, 10.0f, MIN_DISTANCE, MAX_DISTANCE, "%.0f");
			if (m_parameter.isOrtho)
			{
				ImGui::SetItemTooltip("Orthographic: does not change the view size. Use Zoom instead.");
			}
			ImGui::DragFloat("Height", &m_parameter.height, 5.0f, -2000.0f, 2000.0f, "%.0f");
			ImGui::DragFloat("Target Offset Y", &m_parameter.targetOffsetY, 1.0f, -1000.0f, 1000.0f, "%.0f");
			ImGui::DragFloat("Smoothing", &m_parameter.smoothing, 0.1f, 0.1f, 30.0f, "%.1f");

			// ---- 描画の範囲 ----
			ImGui::DragFloat("Near", &m_parameter.nearClip, 1.0f, 0.1f, 1000.0f, "%.1f");
			ImGui::DragFloat("Far", &m_parameter.farClip, 100.0f, 100.0f, 100000.0f, "%.0f");
			ImGui::SetItemTooltip("Objects farther than this from the camera are not drawn.");

			ImGui::Text("Target: (%.0f, %.0f, %.0f)", m_currentTarget.x, m_currentTarget.y, m_currentTarget.z);
			if (ImGui::Button("Reset Camera"))
			{
				m_parameter = m_initialParameter;
			}
			ImGui::SetItemTooltip("Back to the values given in Init.");

			ImGui::PopID();
		}
	}
}
