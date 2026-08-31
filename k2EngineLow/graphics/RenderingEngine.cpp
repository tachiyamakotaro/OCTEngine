#include "k2EngineLowPreCompile.h"

#include "RenderingEngine.h"

namespace nsK2EngineLow
{
	RenderingEngine* RenderingEngine::m_instance = nullptr;

	RenderingEngine::RenderingEngine()
	{
		float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_shadowMap.Create(
			2048,
			2048,
			1,
			1,
			DXGI_FORMAT_R32_TYPELESS,
			DXGI_FORMAT_D32_FLOAT,
			clearColor
		);

		m_lightCamera.SetPosition(-50, 500, 0);
		m_lightCamera.SetTarget(0, 0, 0);
		m_lightCamera.SetUp(1, 0, 0);

		m_lightCamera.SetViewAngle(Math::DegToRad(20.0f));

		m_lightCamera.Update();

		ModelInitData shadowCasterInitData;
		shadowCasterInitData.m_fxFilePath = "Assets/shader/drawShadowMap.fx";
		shadowCasterInitData.m_tkmFilePath = "Assets/shader/unityChan.tkm";

		for (auto shadowCaster : m_shadowCasters)
		{
			shadowCaster->Init(shadowCasterInitData);
		}
	}

	void RenderingEngine::Execute(RenderContext& rc)
	{
		for (auto modelRender : m_renderObjects)
		{
			rc.WaitUntilToPossibleSetRenderTarget(m_shadowMap);
			rc.SetRenderTargetAndViewport(m_shadowMap);
			rc.ClearRenderTargetView(m_shadowMap);

			modelRender->Draw(rc, m_lightCamera);

			rc.WaitUntilFinishDrawingToRenderTarget(m_shadowMap);
		}

		m_renderObjects.clear();
	}

}