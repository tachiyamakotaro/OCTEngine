#pragma once

#include <unordered_set>

namespace nsK2EngineLow
{
	class RenderingEngine
	{
	public:
		static RenderingEngine* GetInstance()
		{
			if (m_instance == nullptr)
			{
				m_instance = new RenderingEngine();
			}
			return m_instance;
		}

		Camera& GetLightCamera()
		{
			return m_lightCamera;
		}

		Texture& GetShadowMapTexture()
		{
			return m_shadowMap.GetRenderTargetTexture();
		}

		void AddRenderObject(Model& model)
		{
			m_renderObjects.push_back(&model);
		}

		void AddShadowCaster(Model& model)
		{
			m_shadowCasters.push_back(&model);
		}

		void Execute(RenderContext& rc);

	private:
		RenderingEngine();

		std::vector<Model*> m_renderObjects;
		//std::unordered_set<Model*> m_shadowMapBoundModels; // SRVを設定済みのモデル

		Camera m_lightCamera;
		RenderTarget m_shadowMap;
		std::vector<Model*> m_shadowCasters;

		static RenderingEngine* m_instance;
	};
}