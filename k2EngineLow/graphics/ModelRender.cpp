#include "k2EngineLowPreCompile.h"

#include "ModelRender.h"


namespace nsK2EngineLow
{
	void ModelRender::Init(const char* tkmFilePath, bool shadowCaster, bool shadowReceiver, void* expandConstantBuffer, const int expandConstantBufferSize)
	{
		ModelInitData initData;
		initData.m_tkmFilePath = tkmFilePath;
		initData.m_fxFilePath = "Assets/shader/model.fx";
		initData.m_expandConstantBuffer = expandConstantBuffer;
		initData.m_expandConstantBufferSize = expandConstantBufferSize;

		m_shadowReceiver = shadowReceiver;
		initData.m_psEntryPointFunc = m_shadowReceiver ? "PSMainShadowReceiver" : "PSMain";

		// model.fx は常にt10(shadowMap)を要求するので、常にバインドしておく。
		initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();

		m_model.Init(initData);

		m_shadowCaster = shadowCaster;

		if (m_shadowCaster == true)
		{
			ModelInitData shadowInitData;
			shadowInitData.m_tkmFilePath = tkmFilePath;
			shadowInitData.m_fxFilePath = "Assets/shader/drawShadowMap.fx";
			m_shadowModel.Init(shadowInitData);
		}
	}

	void ModelRender::Update()
	{
		m_model.UpdateWorldMatrix(m_position, m_rotation, m_scale);

		if (m_shadowCaster == true)
		{
			m_shadowModel.UpdateWorldMatrix(m_position, m_rotation, m_scale);
		}
	}

	void ModelRender::Draw(RenderContext& rc)
	{
		if (m_shadowCaster == true)
		{
			RenderingEngine::GetInstance()->AddShadowCaster(m_shadowModel);
		}
		RenderingEngine::GetInstance()->AddRenderObject(m_model);
	}
}