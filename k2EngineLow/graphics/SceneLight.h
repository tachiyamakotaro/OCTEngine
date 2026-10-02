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
			float shadowBias;
			float pad3;
			float pad4;
			Vector3 ptPosition;
			float ptRange;
			Vector3 ptColor;
			float pad5;
			Matrix mLVP;
		};
		LightData m_light;

	public:
		void Init();

		static SceneLight* GetInstance()
		{
			if (m_instance == nullptr)
			{
				m_instance = new SceneLight();
				m_instance->Init();
			}
			return m_instance;
		}

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
		void SetShadowBias(float shadowBias)
		{
			m_light.shadowBias = shadowBias;
		}
		void SetSpecIntensity(float specIntensity)
		{
			m_light.specIntensity = specIntensity;
		}


	private:
		SceneLight() {}

		static SceneLight* m_instance;
	};
}