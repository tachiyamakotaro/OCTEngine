#pragma once

#include <unordered_set>

namespace nsK2EngineLow
{
	class RenderingEngine
	{
	public:
		static RenderingEngine* GetInstance()
		{
			if (m_instance == nullptr)
			{
				m_instance = new RenderingEngine();
			}
			return m_instance;
		}

		Camera& GetLightCamera()
		{
			return m_lightCamera;
		}

		Texture& GetShadowMapTexture()
		{
			return m_shadowBlur.GetBokeTexture();   // VSM：ぼかし後のシャドウマップを渡す
		}

		void AddRenderObject(Model& model)
		{
			m_models.push_back(&model);
		}

		void AddShadowCaster(Model& model)
		{
			m_shadowCasters.push_back(&model);
		}

		float& GetScreenBlurPower()
		{
			return m_screenBlurPower;
		}

		float& GetBloomThreshold()
		{
			return m_bloomThreshold;
		}

		float& GetBloomIntensity()
		{
			return m_bloomIntensity;
		}

		bool& IsEnableBloom()
		{
			return m_isEnableBloom;
		}

		bool& IsEnableDof()
		{
			return m_isEnableDof;
		}

		float& GetFocusDistance()
		{
			return m_dofParam.focusDistance;
		}

		float& GetFocusRange()
		{
			return m_dofParam.focusRange;
		}

		void Execute(RenderContext& rc);

	private:
		RenderingEngine();

		std::vector<Model*> m_models;
		//std::unordered_set<Model*> m_shadowMapBoundModels; // SRVを設定済みのモデル

		Camera m_lightCamera;

		RenderTarget m_mainRenderTarget;
		Sprite m_copyToFrameBufferSprite;
		Sprite m_copyBlurToFrameBufferSprite;
		GaussianBlur m_screenBlur;              // スクリーンのぼかし
		float m_screenBlurPower = 0.0f;

		RenderTarget m_luminanceRenderTarget;
		Sprite m_luminanceSprite;
		Sprite m_bloomAddSprite;

		DualBlur m_bloomBlur;                          // ブルームのぼかし
		DualBlur m_dofBlur;                            // DOFのぼかし

		struct DofParam
		{
			float focusDistance = 500.0f;
			float focusRange = 100.0f;
		};
		DofParam m_dofParam;
		Sprite m_dofSprite;
		bool m_isEnableDof = false;

		bool m_isEnableBloom = true;                 // ブルームを有効にするかどうか
		float m_bloomThreshold = 1.0f;
		float m_bloomIntensity = 1.0f;

		RenderTarget m_shadowMap;
		GaussianBlur m_shadowBlur;              // VSM：シャドウマップのぼかし
		float m_shadowBlurPower = 2.5f;         // ぼかしの強さ（Phase 4 で imgui につなぐ候補）
		std::vector<Model*> m_shadowCasters;

		static RenderingEngine* m_instance;
	};
}
