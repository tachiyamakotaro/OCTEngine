#include "k2EngineLowPreCompile.h"

#include "DualBlur.h"

namespace nsK2EngineLow
{
	void DualBlur::Init(Texture* originalTexture)
	{
		int w = originalTexture->GetWidth();
		int h = originalTexture->GetHeight();
		for (int i = 0; i < NUM_DOWN; i++)
		{
			w /= 2;
			h /= 2;
			m_downRT[i].Create(w, h, 1, 1, DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_UNKNOWN);
		}
		for (int i = 0; i < NUM_UP; i++)
		{
			w *= 2;
			h *= 2;
			m_upRT[i].Create(w, h, 1, 1, DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_UNKNOWN);
		}

		Texture* src = originalTexture;
		for (int i = 0; i < NUM_DOWN; i++)
		{
			InitSprite(m_downSprite[i], src, "Assets/shader/dualBlurDown.fx", m_downRT[i]);
			src = &m_downRT[i].GetRenderTargetTexture();
		}
		for (int i = 0; i < NUM_UP; i++)
		{
			InitSprite(m_upSprite[i], src, "Assets/shader/dualBlurUp.fx", m_upRT[i]);
			src = &m_upRT[i].GetRenderTargetTexture();
		}
	}


	void DualBlur::InitSprite(Sprite& sprite, Texture* srcTexture,
		const char* fxFilePath, RenderTarget& targetRT)
	{
		SpriteInitData initData;
		initData.m_textures[0] = srcTexture;
		initData.m_fxFilePath = fxFilePath;
		initData.m_width = targetRT.GetWidth();
		initData.m_height = targetRT.GetHeight();
		initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		sprite.Init(initData);
	}

	void DualBlur::ExecuteOnGPU(RenderContext& rc)
	{
		for (int i = 0; i < NUM_DOWN; i++)
		{
			rc.WaitUntilToPossibleSetRenderTarget(m_downRT[i]);
			rc.SetRenderTargetAndViewport(m_downRT[i]);
			m_downSprite[i].Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
			m_downSprite[i].Draw(rc);
			rc.WaitUntilFinishDrawingToRenderTarget(m_downRT[i]);
		}
		for (int i = 0; i < NUM_UP; i++)
		{
			rc.WaitUntilToPossibleSetRenderTarget(m_upRT[i]);
			rc.SetRenderTargetAndViewport(m_upRT[i]);
			m_upSprite[i].Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
			m_upSprite[i].Draw(rc);
			rc.WaitUntilFinishDrawingToRenderTarget(m_upRT[i]);
		}
	}

}
