#include "k2EngineLowPreCompile.h"

#include "ModelRender.h"


namespace nsK2EngineLow
{
	void ModelRender::Init(const char* tkmFilePath, void* expandConstantBuffer, const int expandConstantBufferSize)
	{
		ModelInitData initData;
		initData.m_tkmFilePath = tkmFilePath;
		initData.m_fxFilePath = "Assets/shader/model.fx";
		initData.m_expandConstantBuffer = expandConstantBuffer;
		initData.m_expandConstantBufferSize = expandConstantBufferSize;

		m_model.Init(initData);
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