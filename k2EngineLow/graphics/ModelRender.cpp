#include "k2EngineLowPreCompile.h"

#include "ModelRender.h"


namespace nsK2EngineLow
{
	void ModelRender::Init(const char* tkmFilePath, const char* fxFilePath, void* expandConstantBuffer, const int expandConstantBufferSize)
	{
		ModelInitData initData;
		initData.m_tkmFilePath = tkmFilePath;
		initData.m_fxFilePath = fxFilePath;
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
		m_model.Draw(rc);
	}
}