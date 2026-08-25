#include "k2EngineLowPreCompile.h"

#include "SceneLight.h"


namespace nsK2EngineLow
{
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

	}
}