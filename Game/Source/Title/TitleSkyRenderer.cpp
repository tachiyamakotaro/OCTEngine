/**
 * @file TitleSkyRenderer.cpp
 * @brief タイトル画面の夕暮れの空
 */
#include "stdafx.h"
#include "TitleSkyRenderer.h"


namespace
{
	/** 板のモデル（地形と同じ箱。Z が上・原点が底面・1辺 100） */
	const char* const SKY_MODEL_PATH = "Assets/modelData/Box.tkm";
	constexpr float BOX_SIZE = 100.0f;
	/** 板の厚さ（箱の奥行きにかける値） */
	constexpr float SKY_THICKNESS_SCALE = 0.01f;
	/** 空のシェーダー */
	const char* const SKY_SHADER_PATH = "Assets/shader/titleSky.fx";
	const char* const SKY_PS_ENTRY = "PSMainTitleSky";

	/** float 3つの色を Vector4 にする（a に別の値を入れる） */
	Vector4 ToVector4(const float (&rgb)[3], const float a)
	{
		return Vector4(rgb[0], rgb[1], rgb[2], a);
	}
}


namespace app
{
	namespace title
	{
		void TitleSkyRenderer::Init(const RuinEntranceLayout& layout, const SkyStyle& style)
		{
			m_style = style;
			m_time = 0.0f;
			m_constants.light = SceneLight::GetInstance()->GetSceneLight();
			WriteStyle();

			ModelInitData initData;
			initData.m_tkmFilePath = SKY_MODEL_PATH;
			initData.m_fxFilePath = SKY_SHADER_PATH;
			initData.m_vsEntryPointFunc = "VSMain";
			initData.m_vsSkinEntryPointFunc = "VSMain";
			initData.m_psEntryPointFunc = SKY_PS_ENTRY;
			initData.m_modelUpAxis = enModelUpAxisZ;
			// 板の表と裏を気にしなくてよいように、両面を描く
			initData.m_cullMode = D3D12_CULL_MODE_NONE;
			// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
			initData.m_expandConstantBuffer = &m_constants;
			initData.m_expandConstantBufferSize = sizeof(Constants);
			// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
			initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
			m_model.Init(initData);

			// カメラの注視点の奥に、映る範囲より十分大きく置く（原点は底面の真ん中）
			const float scale = layout.skySize / BOX_SIZE;
			const Vector3 position(layout.cameraTargetX, layout.cameraTargetY - layout.skySize * 0.5f, layout.skyZ);
			m_model.UpdateWorldMatrix(position, Quaternion::Identity, Vector3(scale, scale, SKY_THICKNESS_SCALE));
			m_isInited = true;
		}


		void TitleSkyRenderer::Update(const float deltaTime)
		{
			m_time += deltaTime;
			m_constants.values.screen = Vector4(
				1.0f / static_cast<float>(FRAME_BUFFER_W),
				1.0f / static_cast<float>(FRAME_BUFFER_H),
				m_time,
				0.0f);
		}


		void TitleSkyRenderer::Draw(RenderContext& rc)
		{
			if (!m_isInited)
			{
				return;
			}
			// 自分の定数バッファを持っているので、ライトの値（カメラの位置など）を写す
			m_constants.light = SceneLight::GetInstance()->GetSceneLight();
			RenderingEngine::GetInstance()->AddRenderObject(m_model);
		}


		void TitleSkyRenderer::WriteStyle()
		{
			const SkyStyle& s = m_style;
			ShaderValues& v = m_constants.values;
			v.topColor = ToVector4(s.topColor, 0.0f);
			v.middleColor = ToVector4(s.middleColor, s.middleV);
			v.horizonColor = ToVector4(s.horizonColor, s.horizonV);
			v.screen = Vector4(1.0f / static_cast<float>(FRAME_BUFFER_W), 1.0f / static_cast<float>(FRAME_BUFFER_H), 0.0f, 0.0f);
			v.sun = Vector4(s.sunU, s.sunV, s.sunSize, s.sunGlowSize);
			v.sunColor = ToVector4(s.sunColor, s.sunGlow);
			v.farMountain = ToVector4(s.farMountainColor, s.farMountainBaseV);
			v.nearMountain = ToVector4(s.nearMountainColor, s.nearMountainBaseV);
			v.mountainShape = Vector4(s.farMountainHeight, s.nearMountainHeight, s.hazeAmount, s.starBrightness);
		}
	}
}
