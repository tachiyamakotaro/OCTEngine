#pragma once



namespace nsK2EngineLow
{
	class DualBlur
	{
	public:

		void Init(Texture* originalTexture);

		void ExecuteOnGPU(RenderContext& rc);

		Texture& GetResultTexture()
		{
			return m_upRT[NUM_UP - 1].GetRenderTargetTexture();
		}

	private:

		void InitSprite(Sprite& sprite, Texture* srcTexture,
			const char* fxFilePath, RenderTarget& targetRT);

		static const int NUM_DOWN = 4;
		static const int NUM_UP = 3;

		RenderTarget m_downRT[NUM_DOWN];
		RenderTarget m_upRT[NUM_UP];
		Sprite m_downSprite[NUM_DOWN];
		Sprite m_upSprite[NUM_UP];
	};
}
