/**
 * @file DarknessRenderer.cpp
 * @brief 暗がり（DarkZone）を、黒い半透明の板で描く
 */
#include "stdafx.h"
#include "DarknessRenderer.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/LightTracer.h"


namespace
{
	/** シェーダー */
	const char* const SHADER_PATH = "Assets/shader/darkness.fx";
	/** ピクセルシェーダー */
	const char* const PS_ENTRY = "PSMainDarkness";


	void Set4(float out[4], const float x, const float y, const float z, const float w)
	{
		out[0] = x;
		out[1] = y;
		out[2] = z;
		out[3] = w;
	}


	/** 矩形どうしが重なるか（接しているだけでも重なりとみなす） */
	bool Touches(const app::math::Rect& a, const app::math::Rect& b)
	{
		return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y && a.max.y >= b.min.y;
	}
}


namespace app
{
	namespace render
	{
		DarknessRenderer::DarknessRenderer()
			: m_config()
			, m_zones()
			, m_zoneCount(0)
			, m_isInited(false)
		{}


		void DarknessRenderer::Init(const Config& config)
		{
			m_config = config;
			m_isInited = true;
		}


		void DarknessRenderer::Update(const stage::Stage& stage, const math::Vec2& playerCenter, const bool isOrtho)
		{
			m_zoneCount = 0;
			if (!m_isInited)
			{
				return;
			}

			const Config& c = m_config;
			const float unit = math::Max(c.modelSize, 0.01f);
			const Vector3& forward = g_camera3D->GetForward();
			const std::vector<stage::LightSegment>& segments = stage.GetLightResult().segments;

			for (const stage::DarkZone& zone : stage.GetGimmicks().View<stage::DarkZone>())
			{
				if (zone.darkness <= 0.0f)
				{
					continue;
				}
				ZoneModel& model = AcquireZone();
				Constants& k = model.constants;

				// 暗がりの値
				Set4(k.param, math::Clamp01(zone.darkness), math::Max(zone.lightRadius, 0.0f), math::Max(zone.playerLightRadius, 0.0f), math::Max(zone.edgeFeather, 0.0f));
				Set4(k.view, forward.x, forward.y, forward.z, isOrtho ? 1.0f : 0.0f);
				Set4(k.zone, zone.rect.min.x, zone.rect.min.y, zone.rect.max.x, zone.rect.max.y);
				Set4(k.color, c.color[0], c.color[1], c.color[2], math::Clamp01(c.softness));
				Set4(k.playerLight, math::Max(zone.playerLightRadius, 0.0f), math::Clamp01(zone.playerLightSoftness), 0.0f, 0.0f);

				// 光の線分（暗がりのまわり＝光の半径ぶん広げた範囲に入っているものだけ渡す）
				const float radius = math::Max(zone.lightRadius, 0.0f);
				math::Rect area = zone.rect;
				area.min = area.min - math::Vec2(radius, radius);
				area.max = area.max + math::Vec2(radius, radius);
				int segmentCount = 0;
				for (const stage::LightSegment& segment : segments)
				{
					if (segmentCount >= MAX_SEGMENTS)
					{
						break;
					}
					const math::Rect bounds = {
						{ math::Min(segment.start.x, segment.end.x), math::Min(segment.start.y, segment.end.y) },
						{ math::Max(segment.start.x, segment.end.x), math::Max(segment.start.y, segment.end.y) } };
					if (!Touches(bounds, area))
					{
						continue;
					}
					Set4(k.segments[segmentCount], segment.start.x, segment.start.y, segment.end.x, segment.end.y);
					++segmentCount;
				}

				// 見えた暗がりの足場（暗がりと重なっているものだけ）
				int rectCount = 0;
				for (const stage::HiddenFloor& floor : stage.GetGimmicks().View<stage::HiddenFloor>())
				{
					if (rectCount >= MAX_RECTS)
					{
						break;
					}
					if (!floor.IsVisible() || !Touches(floor.rect, zone.rect))
					{
						continue;
					}
					Set4(k.rects[rectCount],
						floor.rect.min.x - c.rectMargin, floor.rect.min.y - c.rectMargin,
						floor.rect.max.x + c.rectMargin, floor.rect.max.y + c.rectMargin);
					Set4(k.rectVisibility[rectCount], math::Clamp01(floor.visibility), 0.0f, 0.0f, 0.0f);
					++rectCount;
				}

				Set4(k.player, playerCenter.x, playerCenter.y, static_cast<float>(segmentCount), static_cast<float>(rectCount));

				// 暗がりの範囲の手前に、薄い板を置く（板は端のぼかしの分だけ広くしておく）
				const float feather = math::Max(zone.edgeFeather, 0.0f);
				const math::Vec2 center = (zone.rect.min + zone.rect.max) * 0.5f;
				const float width = zone.rect.Width() + feather * 2.0f;
				const float height = zone.rect.Height() + feather * 2.0f;
				model.model.UpdateWorldMatrix(
					Vector3(center.x, center.y, c.planeZ),
					g_quatIdentity,
					Vector3(width / unit, height / unit, math::Max(c.thickness, 0.01f) / unit));
			}
		}


		void DarknessRenderer::Draw(RenderContext& rc)
		{
			(void)rc;
			// エンジンは登録した順に描く。半透明なので、ほかのモデルのあと、光の前に登録する
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			for (size_t i = 0; i < m_zoneCount; ++i)
			{
				m_zones[i]->constants.light = light;
				RenderingEngine::GetInstance()->AddRenderObject(m_zones[i]->model);
			}
		}


		DarknessRenderer::ZoneModel& DarknessRenderer::AcquireZone()
		{
			// Model の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す
			if (m_zoneCount >= m_zones.size())
			{
				auto zone = std::make_unique<ZoneModel>();
				zone->constants = Constants();

				ModelInitData initData;
				initData.m_tkmFilePath = m_config.modelPath.c_str();
				initData.m_fxFilePath = SHADER_PATH;
				initData.m_vsEntryPointFunc = "VSMain";
				initData.m_vsSkinEntryPointFunc = "VSMain";
				initData.m_psEntryPointFunc = PS_ENTRY;
				initData.m_modelUpAxis = enModelUpAxisY;
				initData.m_cullMode = D3D12_CULL_MODE_BACK;
				// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
				initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
				// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
				initData.m_expandConstantBuffer = &zone->constants;
				initData.m_expandConstantBufferSize = sizeof(Constants);
				// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
				initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
				// 半透明：後ろの絵と混ぜる。深度は書かない
				initData.m_alphaBlendMode = AlphaBlendMode_Trans;
				initData.m_isDepthWrite = false;
				initData.m_isDepthTest = true;
				zone->model.Init(initData);

				m_zones.push_back(std::move(zone));
			}
			return *m_zones[m_zoneCount++];
		}
	}
}
