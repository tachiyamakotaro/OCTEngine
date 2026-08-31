#include "k2EngineLowPreCompile.h"

#include "RenderingEngine.h"

namespace nsK2EngineLow
{
	RenderingEngine* RenderingEngine::m_instance = nullptr;

	void RenderingEngine::Init()
	{}

	void RenderingEngine::Update()
	{

	}

	void RenderingEngine::Draw(RenderContext& rc)
	{
		for (auto modelRender : m_renderObjects)
		{
			modelRender->Draw(rc);
		}
	}

	void RenderingEngine::Execute(RenderContext& rc)
	{
		Update();
		Draw(rc);
		m_renderObjects.clear();
	}

}