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
			DXGI_FORMAT_R32G32_FLOAT,   // VSM：(深度, 深度の2乗)
			DXGI_FORMAT_D32_FLOAT,
			clearColor
		);
		// シャドウマップを Create した後に初期化する
		m_shadowBlur.Init(&m_shadowMap.GetRenderTargetTexture());

		// 位置と注視点は SceneLight::Update でライト方向から毎フレーム決める
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

		// VSM：シャドウマップをぼかす（画面に戻す前に）
		m_shadowBlur.ExecuteOnGPU(rc, m_shadowBlurPower);

		g_graphicsEngine->ChangeRenderTargetToFrameBuffer(rc);
		rc.SetViewportAndScissor(g_graphicsEngine->GetFrameBufferViewport());

		for (auto model : m_renderObjects)
		{
			model->Draw(rc);
		}

		m_renderObjects.clear();
	}

}
