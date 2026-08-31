#include "k2EngineLowPreCompile.h"

#include "RenderingEngine.h"

namespace nsK2EngineLow
{
	RenderingEngine* RenderingEngine::m_instance = nullptr;

	void RenderingEngine::Execute(RenderContext& rc)
	{

		for (auto modelRender : m_renderObjects)
		{
			modelRender->Draw(rc);
		}

		m_renderObjects.clear();
	}

}