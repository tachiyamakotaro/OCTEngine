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

		float mainClearColor[4] = { 0.5f, 0.5f, 0.5f, 100000.0f };
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


		float luminanceClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_luminanceRenderTarget.Create(
			FRAME_BUFFER_W,
			FRAME_BUFFER_H,
			1,
			1,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_D32_FLOAT,
			luminanceClearColor
		);

		// 輝度抽出：mainRT → luminanceRT
		{
			SpriteInitData initData;
			initData.m_textures[0] = &m_mainRenderTarget.GetRenderTargetTexture();
			initData.m_fxFilePath = "Assets/shader/samplingLuminance.fx";
			initData.m_width = m_luminanceRenderTarget.GetWidth();
			initData.m_height = m_luminanceRenderTarget.GetHeight();
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			initData.m_expandConstantBuffer = &m_bloomThreshold;
			initData.m_expandConstantBufferSize = sizeof(m_bloomThreshold);
			m_luminanceSprite.Init(initData);
		}

		// デュアルブラー：渡す元テクスチャだけが違う！
		m_bloomBlur.Init(&m_luminanceRenderTarget.GetRenderTargetTexture());   // 輝度抽出の後の絵
		m_dofBlur.Init(&m_mainRenderTarget.GetRenderTargetTexture());          // ★mainRT を直接（輝度抽出を通さない）

		{
			SpriteInitData initData;
			initData.m_textures[0] = &m_mainRenderTarget.GetRenderTargetTexture();   // t0：くっきり版（α = 距離）
			initData.m_textures[1] = &m_dofBlur.GetResultTexture();                  // t1：ボケ版
			initData.m_fxFilePath = "Assets/shader/dof.fx";
			initData.m_width = FRAME_BUFFER_W;
			initData.m_height = FRAME_BUFFER_H;
			initData.m_expandConstantBuffer = &m_dofParam;
			initData.m_expandConstantBufferSize = sizeof(m_dofParam);
			m_dofSprite.Init(initData);
		}

		// 加算合成：ブルームのぼかし結果を mainRT に足す
		{
			SpriteInitData initData;
			initData.m_textures[0] = &m_bloomBlur.GetResultTexture();   // ★DualBlur の結果
			initData.m_fxFilePath = "Assets/shader/sprite.fx";
			initData.m_width = FRAME_BUFFER_W;
			initData.m_height = FRAME_BUFFER_H;
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			initData.m_alphaBlendMode = AlphaBlendMode_Add;     // 上書きではなく「足す」
			m_bloomAddSprite.Init(initData);
		}

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

		if (m_isEnableBloom)
		{
			// ① 輝度抽出：mainRT → luminanceRT
			rc.WaitUntilToPossibleSetRenderTarget(m_luminanceRenderTarget);
			rc.SetRenderTargetAndViewport(m_luminanceRenderTarget);
			rc.ClearRenderTargetView(m_luminanceRenderTarget);      // clip で捨てた所を黒にするため必須
			m_luminanceSprite.Draw(rc);
			rc.WaitUntilFinishDrawingToRenderTarget(m_luminanceRenderTarget);

			// ②③ デュアルブラー（down × 4 → up × 3）
			m_bloomBlur.ExecuteOnGPU(rc);

			// ④ 加算合成：描き先を mainRT に戻して、ぼかした光を足す
			rc.WaitUntilToPossibleSetRenderTarget(m_mainRenderTarget);
			rc.SetRenderTargetAndViewport(m_mainRenderTarget);

			// ※ここで Clear しない！（せっかく描いたシーンが消える）
			m_bloomAddSprite.SetMulColor(Vector4(m_bloomIntensity, m_bloomIntensity, m_bloomIntensity, 1.0f));
			m_bloomAddSprite.Draw(rc);
			rc.WaitUntilFinishDrawingToRenderTarget(m_mainRenderTarget);
		}

		// 後でポストプロセスのパスを入れていく
		if (m_screenBlurPower > 0.0f)
		{
			m_screenBlur.ExecuteOnGPU(rc, m_screenBlurPower);

			g_graphicsEngine->ChangeRenderTargetToFrameBuffer(rc);
			rc.SetViewportAndScissor(g_graphicsEngine->GetFrameBufferViewport());
			m_copyBlurToFrameBufferSprite.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
			m_copyBlurToFrameBufferSprite.Draw(rc);
		}
		else if (m_isEnableDof)
		{
			// ボケ版を作る（ブルームを足し終わった mainRT をぼかす）
			m_dofBlur.ExecuteOnGPU(rc);

			// くっきり版とボケ版を距離でブレンドして、フレームバッファへ
			g_graphicsEngine->ChangeRenderTargetToFrameBuffer(rc);
			rc.SetViewportAndScissor(g_graphicsEngine->GetFrameBufferViewport());
			m_dofSprite.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
			m_dofSprite.Draw(rc);
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
