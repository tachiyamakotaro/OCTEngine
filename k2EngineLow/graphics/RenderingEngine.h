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
				m_instance->Init();
			}
			return m_instance;
		}

		void Init();
		void Update();
		void Draw(RenderContext& rc);

		void AddRenderObject(ModelRender* modelRender)
		{
			m_renderObjects.push_back(modelRender);
		}

		void Execute(RenderContext& rc);

	private:
		std::vector<ModelRender*> m_renderObjects;

		static RenderingEngine* m_instance;
	};
}