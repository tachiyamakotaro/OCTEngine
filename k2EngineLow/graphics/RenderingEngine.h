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

		void AddRenderObject(Model& modelRender)
		{
			m_renderObjects.push_back(&modelRender);
		}

		void Execute(RenderContext& rc);

	private:
		RenderingEngine() {}


		std::vector<Model*> m_renderObjects;

		static RenderingEngine* m_instance;
	};
}