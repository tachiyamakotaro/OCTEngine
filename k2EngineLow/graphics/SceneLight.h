#pragma once

namespace nsK2EngineLow
{
	class SceneLight
	{

	private:
		struct LightData
		{
			Vector3 ambient;
			float pad0;
			Vector3 direction;
			float pad1;
			Vector3 ligColor;
			float pad2;
			Vector3 eyePos;
			float specPow;
			float specIntensity;
			Vector2 pad3;
		};
		LightData m_light;

	public:
		void Init();

		LightData& GetSceneLight()
		{
			return m_light;
		}

		void Update();

		void SetAmbient(const Vector3& ambient)
		{
			m_light.ambient = ambient;
		}
		void SetDirection(const Vector3& direction)
		{
			m_light.direction = direction;
		}
		void SetLigColor(const Vector3& ligColor)
		{
			m_light.ligColor = ligColor;
		}
		void SetEyePos(const Vector3& eyePos)
		{
			m_light.eyePos = eyePos;
		}
		void SetSpecPow(float specPow)
		{
			m_light.specPow = specPow;
		}
		void SetSpecIntensity(float specIntensity)
		{
			m_light.specIntensity = specIntensity;
		}

	};
}