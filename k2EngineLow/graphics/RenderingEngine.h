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
			return m_shadowBlur.GetBokeTexture();   // VSM：ぼかし後のシャドウマップを渡す
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
		GaussianBlur m_shadowBlur;              // VSM：シャドウマップのぼかし
		float m_shadowBlurPower = 2.5f;         // ぼかしの強さ（Phase 4 で imgui につなぐ候補）
		std::vector<Model*> m_shadowCasters;

		static RenderingEngine* m_instance;
	};
}
