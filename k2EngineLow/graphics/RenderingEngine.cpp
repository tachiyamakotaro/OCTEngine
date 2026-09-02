#include "k2EngineLowPreCompile.h"

#include "RenderingEngine.h"

namespace nsK2EngineLow
{
	RenderingEngine* RenderingEngine::m_instance = nullptr;

	RenderingEngine::RenderingEngine()
	{
		float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_shadowMap.Create(
			1024,
			1024,
			1,
			1,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_D32_FLOAT,
			clearColor
		);

		m_lightCamera.SetPosition(-50, 500, 0);
		m_lightCamera.SetTarget(0, 0, 0);
		m_lightCamera.SetUp(1, 0, 0);
		m_lightCamera.SetWidth(2000.0f);
		m_lightCamera.SetHeight(2000.0f);
		m_lightCamera.SetUpdateProjMatrixFunc(Camera::enUpdateProjMatrixFunc_Ortho);
		m_lightCamera.Update();
	}

	void RenderingEngine::Execute(RenderContext& rc)
	{
		rc.WaitUntilToPossibleSetRenderTarget(m_shadowMap);
		rc.SetRenderTargetAndViewport(m_shadowMap);
		rc.ClearRenderTargetView(m_shadowMap);

		for (auto model : m_shadowCasters)
		{
			model->Draw(rc, m_lightCamera);
		}
		m_shadowCasters.clear();
		rc.WaitUntilFinishDrawingToRenderTarget(m_shadowMap);
		g_graphicsEngine->ChangeRenderTargetToFrameBuffer(rc);
		rc.SetViewportAndScissor(g_graphicsEngine->GetFrameBufferViewport());

		for (auto model : m_renderObjects)
		{
			model->Draw(rc);
		}

		m_renderObjects.clear();
	}

}