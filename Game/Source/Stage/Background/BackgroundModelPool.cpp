/**
 * @file BackgroundModelPool.cpp
 * @brief 同じモデルを何個も置く入れ物
 */
#include "stdafx.h"
#include "BackgroundModelPool.h"


namespace app
{
	namespace stage
	{
		void BackgroundModelPool::Init(const BackgroundModelSetting& setting)
		{
			m_setting = setting;
			m_models.clear();
			m_count = 0;
		}


		void BackgroundModelPool::Place(const Vector3& position, const Vector3& scale)
		{
			if (m_count >= m_models.size())
			{
				auto model = std::make_unique<Model>();

				ModelInitData initData;
				initData.m_tkmFilePath = m_setting.tkmFilePath.c_str();
				initData.m_fxFilePath = m_setting.fxFilePath.c_str();
				initData.m_vsEntryPointFunc = "VSMain";
				initData.m_vsSkinEntryPointFunc = "VSMain";
				initData.m_psEntryPointFunc = m_setting.psEntryPointFunc.c_str();
				initData.m_modelUpAxis = m_setting.upAxis;
				initData.m_cullMode = D3D12_CULL_MODE_BACK;
				// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
				initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
				// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
				initData.m_expandConstantBuffer = m_setting.constants;
				initData.m_expandConstantBufferSize = m_setting.constantsSize;
				// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
				initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
				model->Init(initData);

				m_models.push_back(std::move(model));
			}
			m_models[m_count]->UpdateWorldMatrix(position, Quaternion::Identity, scale);
			++m_count;
		}


		void BackgroundModelPool::Draw()
		{
			// ModelRender::Draw と同じく、描く物として登録する（実際に描くのは RenderingEngine）
			RenderingEngine* renderingEngine = RenderingEngine::GetInstance();
			for (size_t i = 0; i < m_count; ++i)
			{
				renderingEngine->AddRenderObject(*m_models[i]);
			}
		}
	}
}
