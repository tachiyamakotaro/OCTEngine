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

		const int frameW = static_cast<int>(FRAME_BUFFER_W);
		const int frameH = static_cast<int>(FRAME_BUFFER_H);
		for (int i = 0; i < 4; i++)
		{
			m_downRenderTarget[i].Create(
				frameW >> (i + 1),
				frameH >> (i + 1),
				1,
				1,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_D32_FLOAT
			);
		}

		for (int i = 0; i < 3; i++)
		{
			m_upRenderTarget[i].Create(
				frameW >> (3 - i),
				frameH >> (3 - i),
				1,
				1,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_D32_FLOAT
			);
		}

		auto initBloomSprite = [](Sprite& sprite, Texture& src, const char* fxPath, RenderTarget& dst,
			void* expandCB = nullptr, int expandCBSize = 0)
			{
				SpriteInitData initData;
				initData.m_textures[0] = &src;
				initData.m_fxFilePath = fxPath;
				initData.m_width = dst.GetWidth();          // 描き先の RT と同じサイズ
				initData.m_height = dst.GetHeight();
				initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
				initData.m_expandConstantBuffer = expandCB;
				initData.m_expandConstantBufferSize = expandCBSize;
				sprite.Init(initData);
			};

		// 輝度抽出：mainRT → luminanceRT
		initBloomSprite(m_luminanceSprite, m_mainRenderTarget.GetRenderTargetTexture(),
			"Assets/shader/samplingLuminance.fx", m_luminanceRenderTarget,
			&m_bloomThreshold, sizeof(m_bloomThreshold));

		// down：前の段の出力が、次の段の入力
		Texture* src = &m_luminanceRenderTarget.GetRenderTargetTexture();
		for (int i = 0; i < 4; i++)
		{
			initBloomSprite(m_downSprite[i], *src, "Assets/shader/dualBlurDown.fx", m_downRenderTarget[i]);
			src = &m_downRenderTarget[i].GetRenderTargetTexture();
		}

		// up：down の最後（1/16）から始まる
		for (int i = 0; i < 3; i++)
		{
			initBloomSprite(m_upSprite[i], *src, "Assets/shader/dualBlurUp.fx", m_upRenderTarget[i]);
			src = &m_upRenderTarget[i].GetRenderTargetTexture();
		}

		// 加算合成：upRT[2]（1/2）を mainRT に足す
		{
			SpriteInitData initData;
			initData.m_textures[0] = &m_upRenderTarget[2].GetRenderTargetTexture();
			initData.m_fxFilePath = "Assets/shader/sprite.fx";
			initData.m_width = FRAME_BUFFER_W;
			initData.m_height = FRAME_BUFFER_H;
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			initData.m_alphaBlendMode = AlphaBlendMode_Add;     // ⑤：上書きではなく「足す」
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

			// ② down × 4：1/2 → 1/4 → 1/8 → 1/16
			for (int i = 0; i < 4; i++)
			{
				rc.WaitUntilToPossibleSetRenderTarget(m_downRenderTarget[i]);
				rc.SetRenderTargetAndViewport(m_downRenderTarget[i]);
				m_downSprite[i].Draw(rc);
				rc.WaitUntilFinishDrawingToRenderTarget(m_downRenderTarget[i]);   // 次の段が読めるように待つ
			}

			// ③ up × 3：1/8 → 1/4 → 1/2
			for (int i = 0; i < 3; i++)
			{
				rc.WaitUntilToPossibleSetRenderTarget(m_upRenderTarget[i]);
				rc.SetRenderTargetAndViewport(m_upRenderTarget[i]);
				m_upSprite[i].Draw(rc);
				rc.WaitUntilFinishDrawingToRenderTarget(m_upRenderTarget[i]);
			}

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
