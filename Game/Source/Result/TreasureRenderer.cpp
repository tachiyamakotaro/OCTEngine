/**
 * @file TreasureRenderer.cpp
 * @brief 宝物の間の宝箱・金貨・光の宝石・まわりの光を描く
 */
#include "stdafx.h"
#include "TreasureRenderer.h"
#include "CelebrationTimeline.h"
#include "Source/Core/Math/MathUtil.h"
#include <cmath>


namespace
{
	// ---- モデル（Assets/modelData/treasure/。Tools/gen_treasure_models.py で作った） ----
	const char* const CHEST_MODEL_PATH = "Assets/modelData/treasure/treasure_chest.tkm";
	const char* const LID_MODEL_PATH = "Assets/modelData/treasure/treasure_lid.tkm";
	const char* const GEM_MODEL_PATH = "Assets/modelData/treasure/treasure_gem.tkm";
	const char* const RAYS_MODEL_PATH = "Assets/modelData/treasure/treasure_rays.tkm";
	const char* const HALO_MODEL_PATH = "Assets/modelData/treasure/treasure_halo.tkm";
	const char* const COINS_MODEL_PATH = "Assets/modelData/treasure/treasure_coins.tkm";

	// ---- シェーダー ----
	const char* const SHADER_PATH = "Assets/shader/treasure.fx";
	const char* const PROP_PS_ENTRY = "PSMainTreasure";
	const char* const GEM_PS_ENTRY = "PSMainTreasureGem";
	const char* const RAYS_PS_ENTRY = "PSMainTreasureRays";
	const char* const HALO_PS_ENTRY = "PSMainTreasureHalo";

	/** 宝石の光に使うシーンのポイントライトの番号 */
	constexpr int GEM_LIGHT_INDEX = 0;
	/** 丸い光を宝石より少し奥に置く（宝石の前に重ならないように） */
	constexpr float HALO_BACK = 30.0f;
	/** 光の筋は丸い光よりさらに奥 */
	constexpr float RAYS_BACK = 40.0f;
}


namespace app
{
	namespace result
	{
		void TreasureRenderer::Init(const TreasureRoomLayout& layout, const TreasureStyle& style)
		{
			m_layout = layout;
			m_style = style;
			m_constants = Constants();

			InitModel(m_chest, CHEST_MODEL_PATH, PROP_PS_ENTRY, false);
			InitModel(m_lid, LID_MODEL_PATH, PROP_PS_ENTRY, false);
			InitModel(m_coinsLeft, COINS_MODEL_PATH, PROP_PS_ENTRY, false);
			InitModel(m_coinsRight, COINS_MODEL_PATH, PROP_PS_ENTRY, false);
			InitModel(m_gem, GEM_MODEL_PATH, GEM_PS_ENTRY, false);
			InitModel(m_rays, RAYS_MODEL_PATH, RAYS_PS_ENTRY, true);
			InitModel(m_halo, HALO_MODEL_PATH, HALO_PS_ENTRY, true);

			// 動かない物
			const Vector3 chestFoot(m_layout.chestX, m_layout.chestY, m_layout.chestZ);
			m_chest.UpdateWorldMatrix(chestFoot, Quaternion::Identity, Vector3::One);
			m_coinsLeft.UpdateWorldMatrix(chestFoot + Vector3(-m_layout.coinOffsetX, 0.0f, -10.0f), Quaternion::Identity, Vector3::One);
			Quaternion flip;
			flip.SetRotationDegY(180.0f);
			m_coinsRight.UpdateWorldMatrix(chestFoot + Vector3(m_layout.coinOffsetX, 0.0f, -10.0f), flip, Vector3::One);

			m_gemPosition = chestFoot + Vector3(0.0f, treasure_shape::GEM_START_Y, 0.0f);
			m_glowRate = 0.0f;
			m_isGemVisible = false;
			m_isInited = true;
		}


		void TreasureRenderer::InitModel(Model& model, const char* tkmFilePath, const char* psEntryPointFunc, const bool isTranslucent)
		{
			ModelInitData initData;
			initData.m_tkmFilePath = tkmFilePath;
			initData.m_fxFilePath = SHADER_PATH;
			initData.m_vsEntryPointFunc = "VSMain";
			initData.m_vsSkinEntryPointFunc = "VSMain";
			initData.m_psEntryPointFunc = psEntryPointFunc;
			initData.m_modelUpAxis = enModelUpAxisY;
			// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			initData.m_expandConstantBuffer = &m_constants;
			initData.m_expandConstantBufferSize = sizeof(Constants);
			// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
			initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
			if (isTranslucent)
			{
				// 光は両面から見えるようにして、奥の物を隠さない（深度を書かない）
				initData.m_alphaBlendMode = AlphaBlendMode_Trans;
				initData.m_isDepthWrite = false;
				initData.m_cullMode = D3D12_CULL_MODE_NONE;
			}
			model.Init(initData);
		}


		void TreasureRenderer::Update(const CelebrationTimeline& timeline)
		{
			if (!m_isInited)
			{
				return;
			}
			const float time = timeline.GetTime();
			const Vector3 chestFoot(m_layout.chestX, m_layout.chestY, m_layout.chestZ);

			// ---- ふた（奥の辺のちょうつがいを軸に、後ろへ倒れるように開く。X 軸のまわりに + で、手前の辺が上がる） ----
			Quaternion lidRotation;
			lidRotation.SetRotationDegX(m_style.lidOpenAngleDeg * timeline.GetLidOpenRate());
			m_lid.UpdateWorldMatrix(chestFoot + Vector3(0.0f, treasure_shape::CHEST_HEIGHT, treasure_shape::LID_HINGE_Z), lidRotation, Vector3::One);

			// ---- 宝石（箱の中から浮かんで、回りながら上下にゆれる） ----
			const float rise = timeline.GetGemRiseRate();
			m_isGemVisible = rise > 0.0f;
			const float bob = std::sin(time * m_style.gemBobSpeed) * m_style.gemBobHeight * rise;
			m_gemPosition = chestFoot + Vector3(0.0f, treasure_shape::GEM_START_Y + m_style.gemRiseHeight * rise + bob, 0.0f);
			Quaternion gemRotation;
			gemRotation.SetRotationDegY(time * m_style.gemSpinSpeedDeg);
			// 出てくるときに少しずつ大きくなる
			const float gemScale = 0.4f + 0.6f * rise;
			m_gem.UpdateWorldMatrix(m_gemPosition, gemRotation, Vector3(gemScale, gemScale, gemScale));

			// ---- まわりの光（宝石の後ろ。宝石が浮かぶほど大きく・濃く） ----
			m_glowRate = rise;
			Quaternion rayRotation;
			rayRotation.SetRotationDegZ(time * m_style.raySpinSpeedDeg);
			const float rayScale = m_style.rayLength * rise;
			m_rays.UpdateWorldMatrix(m_gemPosition + Vector3(0.0f, 0.0f, RAYS_BACK), rayRotation, Vector3(rayScale, rayScale, 1.0f));
			const float haloScale = m_style.haloSize * (0.5f + 0.5f * rise);
			m_halo.UpdateWorldMatrix(m_gemPosition + Vector3(0.0f, 0.0f, HALO_BACK), Quaternion::Identity, Vector3(haloScale, haloScale, 1.0f));

			// ---- シェーダーに渡す値 ----
			m_constants.values.gemColor = m_style.gemColor.ToShaderValue();
			m_constants.values.rayColor = m_style.rayColor.ToShaderValue();
			m_constants.values.param = Vector4(time, m_glowRate, 0.0f, 0.0f);
		}


		void TreasureRenderer::ApplyGemLight() const
		{
			auto& light = SceneLight::GetInstance()->GetSceneLight();
			auto& point = light.ptLights[GEM_LIGHT_INDEX];
			point.position = m_gemPosition;
			point.range = m_style.lightRange;
			const float intensity = m_style.lightIntensity * m_glowRate;
			point.color = Vector3(m_style.lightColor[0] * intensity, m_style.lightColor[1] * intensity, m_style.lightColor[2] * intensity);
			// 向きのない光（スポットライトにしない）
			point.angle = 0.0f;
			light.numPtLights = m_glowRate > 0.0f ? GEM_LIGHT_INDEX + 1 : 0;
		}


		void TreasureRenderer::ClearGemLight()
		{
			SceneLight::GetInstance()->GetSceneLight().numPtLights = 0;
		}


		void TreasureRenderer::Draw(RenderContext& rc)
		{
			if (!m_isInited)
			{
				return;
			}
			// 自分の定数バッファを持っているので、ライトの値（カメラの位置・ポイントライトなど）を写す
			m_constants.light = SceneLight::GetInstance()->GetSceneLight();

			RenderingEngine* renderingEngine = RenderingEngine::GetInstance();
			renderingEngine->AddRenderObject(m_chest);
			renderingEngine->AddRenderObject(m_lid);
			renderingEngine->AddRenderObject(m_coinsLeft);
			renderingEngine->AddRenderObject(m_coinsRight);
			if (m_isGemVisible)
			{
				renderingEngine->AddRenderObject(m_gem);
			}
		}


		void TreasureRenderer::DrawGlow(RenderContext& rc)
		{
			if (!m_isInited || m_glowRate <= 0.0f)
			{
				return;
			}
			RenderingEngine* renderingEngine = RenderingEngine::GetInstance();
			renderingEngine->AddRenderObject(m_halo);
			renderingEngine->AddRenderObject(m_rays);
		}
	}
}
