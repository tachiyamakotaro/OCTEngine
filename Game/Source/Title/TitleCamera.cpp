/**
 * @file TitleCamera.cpp
 * @brief タイトル画面のカメラ
 */
#include "stdafx.h"
#include "TitleCamera.h"


namespace
{
	/** カメラの近平面と遠平面（GameCamera と同じ） */
	constexpr float CAMERA_NEAR = 10.0f;
	constexpr float CAMERA_FAR = 20000.0f;
}


namespace app
{
	namespace title
	{
		void TitleCamera::Init(const RuinEntranceLayout& layout)
		{
			m_layout = layout;
			Apply();
		}


		void TitleCamera::Apply() const
		{
			const float aspect = static_cast<float>(FRAME_BUFFER_W) / static_cast<float>(FRAME_BUFFER_H);
			g_camera3D->SetNear(CAMERA_NEAR);
			g_camera3D->SetFar(CAMERA_FAR);
			g_camera3D->SetUpdateProjMatrixFunc(Camera::enUpdateProjMatrixFunc_Ortho);
			g_camera3D->SetHeight(m_layout.viewHeight);
			g_camera3D->SetWidth(m_layout.viewHeight * aspect);

			const Vector3 target(m_layout.cameraTargetX, m_layout.cameraTargetY, 0.0f);
			g_camera3D->SetTarget(target);
			g_camera3D->SetPosition(Vector3(target.x, target.y + m_layout.cameraHeight, target.z - m_layout.cameraDistance));
		}
	}
}
