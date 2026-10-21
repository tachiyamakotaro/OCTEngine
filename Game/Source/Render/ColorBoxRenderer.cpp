/**
 * @file ColorBoxRenderer.cpp
 * @brief 色の付いた半透明の箱を、好きな数だけ描く
 */
#include "stdafx.h"
#include "ColorBoxRenderer.h"


namespace
{
	/** シェーダー（光と同じ） */
	const char* const SHADER_PATH = "Assets/shader/lightBeam.fx";
	/** ピクセルシェーダー */
	const char* const PS_ENTRY = "PSMainLightBeam";
	/** 大きさ 0 の箱を作らないための下限 */
	constexpr float MIN_SIZE = 0.01f;


	void Set4(float out[4], const float x, const float y, const float z, const float w)
	{
		out[0] = x;
		out[1] = y;
		out[2] = z;
		out[3] = w;
	}


	float AtLeast(const float v, const float lo)
	{
		return v > lo ? v : lo;
	}
}


namespace app
{
	namespace render
	{
		ColorBoxRenderer::ColorBoxRenderer()
			: m_modelPath()
			, m_modelSize(100.0f)
			, m_boxes()
			, m_count(0)
			, m_isInited(false)
		{}


		void ColorBoxRenderer::Init(const char* modelPath, const float modelSize)
		{
			m_modelPath = modelPath != nullptr ? modelPath : "";
			m_modelSize = AtLeast(modelSize, MIN_SIZE);
			m_isInited = true;
		}


		void ColorBoxRenderer::Begin()
		{
			m_count = 0;
		}


		void ColorBoxRenderer::Put(const Vector3& center, const Vector3& size, const Look& look, const float angleDeg)
		{
			if (!m_isInited)
			{
				return;
			}
			BoxModel& box = Acquire();
			Set4(box.constants.color, look.color[0], look.color[1], look.color[2], look.color[3]);
			Set4(box.constants.param, look.frontOpacity, look.edgeGlow, 0.0f, 0.0f);
			// 切り口は使わない（面の向きが 0 なら切らない）
			Set4(box.constants.startCap, 0.0f, 0.0f, 0.0f, 0.0f);
			Set4(box.constants.endCap, 0.0f, 0.0f, 0.0f, 0.0f);

			Quaternion rotation;
			rotation.SetRotationDegZ(angleDeg);
			const float unit = m_modelSize;
			box.model.UpdateWorldMatrix(
				center,
				rotation,
				Vector3(AtLeast(size.x, MIN_SIZE) / unit, AtLeast(size.y, MIN_SIZE) / unit, AtLeast(size.z, MIN_SIZE) / unit));
		}


		void ColorBoxRenderer::Draw(RenderContext& rc)
		{
			(void)rc;
			// エンジンは登録した順に描く。半透明なので、ほかのモデルのあとに登録する
			// 箱は1つずつ自分の定数バッファを持っているので、ライトの値（カメラの位置など）もそれぞれに写す
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			for (size_t i = 0; i < m_count; ++i)
			{
				m_boxes[i]->constants.light = light;
				RenderingEngine::GetInstance()->AddRenderObject(m_boxes[i]->model);
			}
		}


		ColorBoxRenderer::BoxModel& ColorBoxRenderer::Acquire()
		{
			// Model の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す
			if (m_count >= m_boxes.size())
			{
				auto box = std::make_unique<BoxModel>();
				box->constants = Constants();

				ModelInitData initData;
				initData.m_tkmFilePath = m_modelPath.c_str();
				initData.m_fxFilePath = SHADER_PATH;
				initData.m_vsEntryPointFunc = "VSMain";
				initData.m_vsSkinEntryPointFunc = "VSMain";
				initData.m_psEntryPointFunc = PS_ENTRY;
				initData.m_modelUpAxis = enModelUpAxisY;
				initData.m_cullMode = D3D12_CULL_MODE_BACK;
				// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
				initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
				// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
				initData.m_expandConstantBuffer = &box->constants;
				initData.m_expandConstantBufferSize = sizeof(Constants);
				// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
				initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
				// 半透明：後ろの絵と混ぜる。深度は書かない
				initData.m_alphaBlendMode = AlphaBlendMode_Trans;
				initData.m_isDepthWrite = false;
				initData.m_isDepthTest = true;
				box->model.Init(initData);

				m_boxes.push_back(std::move(box));
			}
			return *m_boxes[m_count++];
		}
	}
}
