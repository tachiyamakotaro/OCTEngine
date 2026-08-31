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

		m_model.Init(initData);

		m_shadowCaster = shadowCaster;
		m_shadowReceiver = shadowReceiver;

		if (m_shadowCaster == true)
		{
			ModelInitData shadowInitData;
			shadowInitData.m_tkmFilePath = tkmFilePath;
			shadowInitData.m_fxFilePath = "Assets/shader/drawShadowMap.fx";

			m_model.Init(shadowInitData);
		}

		if (m_shadowReceiver == true)
		{
			initData.m_fxFilePath = "Assets/shader/model.fx";
			initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
		}
	}

	void ModelRender::Update()
	{
		m_model.UpdateWorldMatrix(m_position, m_rotation, m_scale);
	}

	void ModelRender::Draw(RenderContext& rc)
	{
		RenderingEngine::GetInstance()->AddRenderObject(m_model);
	}
}