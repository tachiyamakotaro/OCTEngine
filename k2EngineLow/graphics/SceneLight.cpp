#include "k2EngineLowPreCompile.h"

#include "RenderingEngine.h"
#include "SceneLight.h"


namespace nsK2EngineLow
{
	SceneLight* SceneLight::m_instance = nullptr;

	void SceneLight::Init()
	{
		m_light.ambient = { 0.6f, 0.6f, 0.6f };
		m_light.direction = { 1.0f, -1.0f, 0.0f };
		m_light.direction.Normalize();
		m_light.ligColor = { 0.6f, 0.7f, 0.4f };
		m_light.specPow = 0.5f;
		m_light.specIntensity = 5.0f;
	}

	void SceneLight::Update()
	{
		m_light.eyePos = g_camera3D->GetPosition();
		//m_light.mLVP = RenderingEngine::GetInstance()->GetLightCamera().GetViewProjectionMatrix();

		// ライトカメラをライト方向に追従させてから LVP を取る（md Step 2-2）
		Camera& lightCamera = RenderingEngine::GetInstance()->GetLightCamera();
		Vector3 lightCameraPos = m_light.direction;
		lightCameraPos.Scale(-1000.0f);            // ライトの向きの逆側に置く
		lightCamera.SetPosition(lightCameraPos);
		lightCamera.SetTarget(0.0f, 0.0f, 0.0f);
		lightCamera.Update();

		m_light.mLVP = lightCamera.GetViewProjectionMatrix();
	}
}