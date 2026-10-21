/**
 * @file ColorLightRenderer.cpp
 * @brief 色の光のギミック（色センサー・扉の色の印・色の付いた光源）を描く
 */
#include "stdafx.h"
#include "ColorLightRenderer.h"
#include <cmath>
#include "Stage.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/** 窓・印を、面から少しだけ手前に浮かせる（面とぴったり重なってちらつかないように） */
			constexpr float SURFACE_OFFSET = 0.5f;
			/** 窓・印の不透明度 */
			constexpr float BOX_OPACITY = 0.95f;
			/** 窓・印の縁の明るさ */
			constexpr float BOX_EDGE_GLOW = 1.3f;
		}


		void ColorLightRenderer::Init(const Config& config)
		{
			m_config = config;
			m_colorBoxes.Init(m_config.boxModelPath.c_str(), m_config.modelSize);
			m_isInited = true;
		}


		void ColorLightRenderer::Update(const Stage& stage, const float deltaTime)
		{
			m_receiverCount = 0;
			m_colorBoxes.Begin();
			if (!m_isInited)
			{
				return;
			}
			const Config& c = m_config;
			m_time += deltaTime;
			// 違う色が当たっているときの点滅（0 → 1 → 0 をくり返す）
			const float blink = 0.5f + 0.5f * std::sin(m_time * c.wrongBlinkHz * 2.0f * math::PI);

			// ---- 色センサー：本体 ＋ 手前の色の窓 ----
			for (const ColorReceiver& receiver : stage.GetGimmicks().View<ColorReceiver>())
			{
				const math::Rect& rect = receiver.rect;
				const math::Vec2 center = (rect.min + rect.max) * 0.5f;

				ModelRender& body = AcquireReceiverModel();
				body.SetPosition(Vector3(center.x, center.y, 0.0f));
				body.SetRotation(Quaternion::Identity);
				body.SetScale(Vector3(rect.Width() / c.modelSize, rect.Height() / c.modelSize, c.receiverDepth / c.modelSize));
				body.Update();

				float brightness = c.offBrightness;
				if (receiver.isMatched)
				{
					brightness = c.onBrightness;
				}
				else if (receiver.IsWrongColor())
				{
					brightness = math::Lerp(c.offBrightness, c.wrongBrightness, blink);
				}
				const float z = -c.receiverDepth * 0.5f - c.lensThickness * 0.5f - SURFACE_OFFSET;
				PutColorBox(center, z, rect.Width() * c.lensRate, rect.Height() * c.lensRate, c.lensThickness, receiver.requiredColor, brightness);
			}

			// ---- 扉：つながった色センサーの色の印（開いている間は描かない）----
			for (const Door& door : stage.GetGimmicks().View<Door>())
			{
				if (!door.IsSolid())
				{
					continue;
				}
				// この扉につながった色センサーを集める
				std::vector<const ColorReceiver*> sensors;
				for (const std::string& signalName : door.requiredReceivers)
				{
					for (const ColorReceiver& receiver : stage.GetGimmicks().View<ColorReceiver>())
					{
						if (receiver.name == signalName)
						{
							sensors.push_back(&receiver);
							break;
						}
					}
				}
				if (sensors.empty())
				{
					continue;
				}

				const math::Rect current = door.GetCurrentRect();
				const float size = math::Min(c.doorMarkSize, current.Width() * 0.8f);
				const float spacing = size * c.doorMarkSpacingRate;
				const float totalHeight = spacing * static_cast<float>(sensors.size() - 1) + size;
				if (current.Height() < totalHeight)
				{
					// ほとんど開いていて印が入らない
					continue;
				}
				const math::Vec2 center = (current.min + current.max) * 0.5f;
				const float z = -c.doorDepth * 0.5f - c.lensThickness * 0.5f - SURFACE_OFFSET;
				for (size_t i = 0; i < sensors.size(); ++i)
				{
					// 上から順に並べる
					const float offsetY = (static_cast<float>(sensors.size() - 1) * 0.5f - static_cast<float>(i)) * spacing;
					const float brightness = sensors[i]->isMatched ? c.onBrightness : c.offBrightness;
					PutColorBox({ center.x, center.y + offsetY }, z, size, size, c.lensThickness, sensors[i]->requiredColor, brightness);
				}
			}

			// ---- 光源：白以外の光を出す光源の手前に、光の色の窓 ----
			for (const EmitRay& source : stage.GetGimmicks().View<EmitRay>())
			{
				if (source.color == LightColor::White)
				{
					continue;
				}
				const float z = -c.lightSourceDepth * 0.5f - c.lensThickness * 0.5f - SURFACE_OFFSET;
				const float brightness = source.isOn ? c.lightSourceBrightness : c.offBrightness;
				PutColorBox(source.position, z, c.lightSourceLensSize, c.lightSourceLensSize, c.lensThickness, source.color, brightness);
			}
		}


		void ColorLightRenderer::DrawOpaque(RenderContext& rc)
		{
			for (size_t i = 0; i < m_receiverCount; ++i)
			{
				m_receiverModels[i]->Draw(rc);
			}
		}


		void ColorLightRenderer::DrawTranslucent(RenderContext& rc)
		{
			m_colorBoxes.Draw(rc);
		}


		void ColorLightRenderer::PutColorBox(
			const math::Vec2& center, const float z, const float width, const float height, const float thickness,
			const LightColor color, const float brightness)
		{
			float rgb[3];
			GetLightColorRGB(color, rgb);
			render::ColorBoxRenderer::Look look;
			look.color[0] = rgb[0] * brightness;
			look.color[1] = rgb[1] * brightness;
			look.color[2] = rgb[2] * brightness;
			look.color[3] = BOX_OPACITY;
			look.frontOpacity = 1.0f;
			look.edgeGlow = BOX_EDGE_GLOW;
			m_colorBoxes.Put(Vector3(center.x, center.y, z), Vector3(width, height, thickness), look);
		}


		ModelRender& ColorLightRenderer::AcquireReceiverModel()
		{
			// ModelRender の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す（GimmickRenderer と同じ）
			if (m_receiverCount >= m_receiverModels.size())
			{
				auto model = std::make_unique<ModelRender>();
				model->Init(
					m_config.receiverModelPath.c_str(), nullptr, 0,
					false, true,
					&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
					m_config.upAxis,
					D3D12_CULL_MODE_BACK);
				m_receiverModels.push_back(std::move(model));
			}
			return *m_receiverModels[m_receiverCount++];
		}
	}
}
