#include "k2EngineLowPreCompile.h"

#include "RenderingEngine.h"

namespace nsK2EngineLow
{
	RenderingEngine* RenderingEngine::m_instance = nullptr;

	namespace
	{
		constexpr float FRAME_BUFFER_W = 1920.0f;
		constexpr float FRAME_BUFFER_H = 1080.0f;
	}

	RenderingEngine::RenderingEngine()
	{
		float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_shadowMap.Create(
			1024,
			1024,
			1,
			1,
			DXGI_FORMAT_R32G32_FLOAT,   // VSM：(深度, 深度の2乗)
			DXGI_FORMAT_D32_FLOAT,
			clearColor
		);
		// シャドウマップを Create した後に初期化する
		m_shadowBlur.Init(&m_shadowMap.GetRenderTargetTexture());

		float mainClearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
		m_mainRenderTarget.Create(
			FRAME_BUFFER_W,
			FRAME_BUFFER_H,
			1,
			1,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_D32_FLOAT,
			mainClearColor
		);
		m_screenBlur.Init(&m_mainRenderTarget.GetRenderTargetTexture());

		SpriteInitData spriteInitData;
		spriteInitData.m_width = FRAME_BUFFER_W;
		spriteInitData.m_height = FRAME_BUFFER_H;
		spriteInitData.m_fxFilePath = "Assets/shader/sprite.fx";
		spriteInitData.m_textures[0] = &m_mainRenderTarget.GetRenderTargetTexture();
		m_copyToFrameBufferSprite.Init(spriteInitData);

		spriteInitData.m_textures[0] = &m_screenBlur.GetBokeTexture();   // VSM：ぼかし後のシャドウマップを渡す
		m_copyBlurToFrameBufferSprite.Init(spriteInitData);

		// 位置と注視点は SceneLight::Update でライト方向から毎フレーム決める
		m_lightCamera.SetUp(1, 0, 0);
		m_lightCamera.SetWidth(2000.0f);
		m_lightCamera.SetHeight(2000.0f);
		m_lightCamera.SetUpdateProjMatrixFunc(Camera::enUpdateProjMatrixFunc_Ortho);
		m_lightCamera.Update();
	}

	void RenderingEngine::Execute(RenderContext& rc)
	{
		rc.WaitUntilToPossibleSetRenderTarget(m_shadowMap);
		rc.SetRenderTargetAndViewport(m_shadowMap);
		rc.ClearRenderTargetView(m_shadowMap);

		for (auto model : m_shadowCasters)
		{
			model->Draw(rc, m_lightCamera);
		}
		m_shadowCasters.clear();
		rc.WaitUntilFinishDrawingToRenderTarget(m_shadowMap);

		// VSM：シャドウマップをぼかす（画面に戻す前に）
		m_shadowBlur.ExecuteOnGPU(rc, m_shadowBlurPower);

		//g_graphicsEngine->ChangeRenderTargetToFrameBuffer(rc);
		//rc.SetViewportAndScissor(g_graphicsEngine->GetFrameBufferViewport());

		rc.WaitUntilToPossibleSetRenderTarget(m_mainRenderTarget);
		rc.SetRenderTargetAndViewport(m_mainRenderTarget);
		rc.ClearRenderTargetView(m_mainRenderTarget);
		for (auto model : m_models)
		{
			model->Draw(rc);
		}
		rc.WaitUntilFinishDrawingToRenderTarget(m_mainRenderTarget);

		// 後でポストプロセスのパスを入れていく
		if (m_screenBlurPower > 0.0f)
		{
			m_screenBlur.ExecuteOnGPU(rc, m_screenBlurPower);

			g_graphicsEngine->ChangeRenderTargetToFrameBuffer(rc);
			rc.SetViewportAndScissor(g_graphicsEngine->GetFrameBufferViewport());
			m_copyBlurToFrameBufferSprite.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
			m_copyBlurToFrameBufferSprite.Draw(rc);
		}
		else
		{
			// ぼかしなしの場合は、メインレンダーターゲットのテクスチャをフレームバッファにコピーする
			g_graphicsEngine->ChangeRenderTargetToFrameBuffer(rc);
			m_copyToFrameBufferSprite.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
			m_copyToFrameBufferSprite.Draw(rc);
		}

		//g_graphicsEngine->ChangeRenderTargetToFrameBuffer(rc);
		//m_copyToFrameBufferSprite.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
		//m_copyToFrameBufferSprite.Draw(rc);

		m_models.clear();
	}

}
