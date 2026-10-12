/**
 * @file MirrorGlass.cpp
 * @brief 本物の世界と鏡の中の世界の境目に置く、巨大な半透明のガラス
 */
#include "stdafx.h"
#include "MirrorGlass.h"

#include "Source/Core/Math/MathUtil.h"
#include "Source/Stage/StageObjects.h"


namespace
{
	/** 板のモデル（1辺100の箱・原点は中心・Y-up。Tools/make_gimmick_models.py で作る） */
	const char* const MODEL_PATH = "Assets/modelData/gimmick/mirror_glass.tkm";
	/** シェーダー */
	const char* const SHADER_PATH = "Assets/shader/mirrorGlass.fx";
	/** モデルの1辺の長さ */
	constexpr float MODEL_SIZE = 100.0f;
	/** 時間がこれを超えたら戻す（float の精度が落ちて光の筋がガタつかないように） */
	constexpr float TIME_WRAP = 10000.0f;


	void Set4(float out[4], const float x, const float y, const float z, const float w)
	{
		out[0] = x;
		out[1] = y;
		out[2] = z;
		out[3] = w;
	}
}


namespace app
{
	namespace render
	{
		MirrorGlass::MirrorGlass()
			: m_model()
			, m_constants()
			, m_config()
			, m_time(0.0f)
			, m_isInited(false)
			, m_isActive(false)
		{}


		void MirrorGlass::Init()
		{
			if (m_isInited)
			{
				return;
			}
			ModelInitData initData;
			initData.m_tkmFilePath = MODEL_PATH;
			initData.m_fxFilePath = SHADER_PATH;
			initData.m_vsEntryPointFunc = "VSMain";
			initData.m_vsSkinEntryPointFunc = "VSMain";
			initData.m_psEntryPointFunc = "PSMain";
			initData.m_modelUpAxis = enModelUpAxisY;
			// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			// 半透明：後ろの絵と混ぜる。深度は書かない（板の奥にあるものを隠さない）
			initData.m_alphaBlendMode = AlphaBlendMode_Trans;
			initData.m_isDepthWrite = false;
			initData.m_isDepthTest = true;
			initData.m_cullMode = D3D12_CULL_MODE_BACK;
			// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
			initData.m_expandConstantBuffer = &m_constants;
			initData.m_expandConstantBufferSize = sizeof(m_constants);
			m_model.Init(initData);
			m_isInited = true;
		}


		void MirrorGlass::Update(const stage::BackMirror& backMirror, const float deltaTime)
		{
			m_isActive = m_config.isVisible && backMirror.isEnabled && m_config.opacity > 0.0f;
			m_time += deltaTime;
			if (m_time > TIME_WRAP)
			{
				m_time -= TIME_WRAP;
			}
			if (!m_isInited || !m_isActive)
			{
				return;
			}

			// 鏡の枠と同じ大きさで、鏡の面の少し手前に置く
			const math::Rect& frame = backMirror.frame;
			const Vector3 position(
				(frame.min.x + frame.max.x) * 0.5f,
				(frame.min.y + frame.max.y) * 0.5f,
				backMirror.planeZ - m_config.zOffset);
			const Vector3 scale(
				frame.Width() / MODEL_SIZE,
				frame.Height() / MODEL_SIZE,
				math::Max(m_config.thickness, 0.01f) / MODEL_SIZE);
			m_model.UpdateWorldMatrix(position, Quaternion(), scale);

			// シェーダーに渡す値
			const Config& c = m_config;
			Set4(m_constants.tintColor, c.tint[0], c.tint[1], c.tint[2], c.tint[3]);
			Set4(m_constants.edgeColor, c.edgeColor[0], c.edgeColor[1], c.edgeColor[2], math::Max(c.edgeWidth, 0.0f));
			Set4(m_constants.sheenColor, c.sheenColor[0], c.sheenColor[1], c.sheenColor[2], c.sheenOpacity);
			Set4(m_constants.rect, frame.min.x, frame.min.y, frame.max.x, frame.max.y);
			const Vector3& eye = g_camera3D->GetPosition();
			Set4(m_constants.eyePosTime, eye.x, eye.y, eye.z, m_time);
			Set4(m_constants.param, c.fresnel, c.topBrightness, math::Max(c.sheenSpacing, 0.0f), c.sheenSpeed);
			Set4(m_constants.panel, math::Max(c.panelSize, 0.0f), math::Max(c.seamWidth, 0.0f), c.seamOpacity, math::Max(c.opacity, 0.0f));
		}


		void MirrorGlass::Draw(RenderContext& rc)
		{
			(void)rc;
			if (!IsDrawn())
			{
				return;
			}
			// エンジンは登録した順に描く。半透明なので最後に登録する
			RenderingEngine::GetInstance()->AddRenderObject(m_model);
		}
	}
}
