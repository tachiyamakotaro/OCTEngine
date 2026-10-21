/**
 * @file ColorGlassRenderer.cpp
 * @brief 色ガラスを半透明の色の板で描く
 */
#include "stdafx.h"
#include "ColorGlassRenderer.h"
#include "Stage.h"
#include "Gimmick/ColorGlass.h"


namespace app
{
	namespace stage
	{
		void ColorGlassRenderer::Init(const Config& config)
		{
			m_config = config;
			m_boxes.Init(m_config.boxModelPath, m_config.modelSize);
			m_isInited = true;
		}


		void ColorGlassRenderer::Update(const Stage& stage)
		{
			m_boxes.Begin();
			if (!m_isInited)
			{
				return;
			}
			const Config& c = m_config;
			for (const ColorGlass& glass : stage.GetGimmicks().View<ColorGlass>())
			{
				float rgb[3];
				GetLightColorRGB(glass.color, rgb);
				const float brightness = glass.isLit ? c.litBrightness : c.brightness;

				render::ColorBoxRenderer::Look look;
				look.color[0] = rgb[0] * brightness;
				look.color[1] = rgb[1] * brightness;
				look.color[2] = rgb[2] * brightness;
				look.color[3] = c.opacity;
				look.frontOpacity = c.frontOpacity;
				look.edgeGlow = c.edgeGlow;

				const math::Vec2 center = (glass.rect.min + glass.rect.max) * 0.5f;
				m_boxes.Put(Vector3(center.x, center.y, 0.0f), Vector3(glass.rect.Width(), glass.rect.Height(), c.depth), look);
			}
		}


		void ColorGlassRenderer::Draw(RenderContext& rc)
		{
			m_boxes.Draw(rc);
		}
	}
}
