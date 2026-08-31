#pragma once


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

		void AddRenderObject(Model& modelRender)
		{
			m_renderObjects.push_back(&modelRender);
		}

		void Execute(RenderContext& rc);

	private:
		RenderingEngine() {}

		std::vector<Model*> m_renderObjects;

		Camera m_lightCamera;
		RenderTarget m_shadowMap;
		std::vector<Model*> m_shadowCasters;

		static RenderingEngine* m_instance;
	};
}