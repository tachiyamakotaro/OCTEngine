#include "k2EngineLowPreCompile.h"

#include "ModelRender.h"


namespace nsK2EngineLow
{
	void ModelRender::Init(const char* tkmFilePath, AnimationClip* animationClips, int numAnimationClips, bool shadowCaster, bool shadowReceiver, void* expandConstantBuffer, const int expandConstantBufferSize, EnModelUpAxis modelUpAxis)
	{
		std::string skeletonFilePath = tkmFilePath;
		skeletonFilePath.replace(skeletonFilePath.length() - 3, 3, "tks");
		m_skeleton.Init(skeletonFilePath.c_str());

		ModelInitData initData;
		initData.m_tkmFilePath = tkmFilePath;
		initData.m_fxFilePath = "Assets/shader/model.fx";
		initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		initData.m_skeleton = &m_skeleton;
		initData.m_vsSkinEntryPointFunc = "VSMainSkin";
		initData.m_modelUpAxis = modelUpAxis;
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
			shadowInitData.m_colorBufferFormat[0] = DXGI_FORMAT_R32G32_FLOAT;
			shadowInitData.m_skeleton = &m_skeleton;
			shadowInitData.m_vsSkinEntryPointFunc = "VSMainSkin";
			shadowInitData.m_modelUpAxis = modelUpAxis;
			m_shadowModel.Init(shadowInitData);
		}

		if (animationClips != nullptr)
		{
			m_animation.Init(m_skeleton, animationClips, numAnimationClips);
			m_isAnimated = true;
		}
	}

	void ModelRender::Update()
	{
		m_model.UpdateWorldMatrix(m_position, m_rotation, m_scale);

		if (m_isAnimated) {
			m_animation.Progress(g_gameTime->GetFrameDeltaTime() * m_animationSpeed);
		}
		if (m_skeleton.IsInited())
		{
			m_skeleton.Update(m_model.GetWorldMatrix());
		}
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