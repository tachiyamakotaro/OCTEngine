/**
 * @file LightBeamRenderer.cpp
 * @brief 光源から出る光を、半透明に少し光らせて描く
 */
#include "stdafx.h"
#include "LightBeamRenderer.h"
#include <cmath>
#include "Source/Core/Math/MathUtil.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/LightTracer.h"


namespace
{
	/** シェーダー */
	const char* const SHADER_PATH = "Assets/shader/lightBeam.fx";
	/** ピクセルシェーダー（半透明で、上下の縁を少し光らせる） */
	const char* const PS_ENTRY = "PSMainLightBeam";
	/** これより短い光の線分は描かない */
	constexpr float MIN_BEAM_LENGTH = 0.5f;
	/** 鏡の面がほとんど光と平行なときに、光の箱を伸ばしすぎないための下限 */
	constexpr float MIN_CAP_COS = 0.2f;


	void Set4(float out[4], const float x, const float y, const float z, const float w)
	{
		out[0] = x;
		out[1] = y;
		out[2] = z;
		out[3] = w;
	}


	/**
	 * @brief 光の端を鏡の面で切るとき、光の箱を端からどれだけ伸ばすか
	 * @details 鏡が斜めだと、光の片側の縁は、光の真ん中が当たった点より先で鏡に届く。
	 *          その分だけ箱を伸ばしておき、鏡の面より先はシェーダーで捨てる。
	 */
	float CalcCapExtension(const app::math::Vec2& direction, const app::math::Vec2& normal, const float halfWidth)
	{
		const float cosAngle = app::math::Abs(app::math::Dot(direction, normal));
		const float sinAngle = std::sqrt(app::math::Max(1.0f - cosAngle * cosAngle, 0.0f));
		return halfWidth * sinAngle / app::math::Max(cosAngle, MIN_CAP_COS);
	}


	/**
	 * @brief 切り口をシェーダーに渡す形にする（面の向きが 0 なら切らない）
	 */
	void SetCap(float out[4], const app::math::Vec2& normal, const app::math::Vec2& point)
	{
		const float lengthSq = normal.LengthSq();
		if (lengthSq < 0.0001f)
		{
			Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
			return;
		}
		const float inv = 1.0f / std::sqrt(lengthSq);
		Set4(out, normal.x * inv, normal.y * inv, point.x, point.y);
	}
}


namespace app
{
	namespace render
	{
		LightBeamRenderer::LightBeamRenderer()
			: m_config()
			, m_beams()
			, m_beamCount(0)
			, m_isInited(false)
		{}


		void LightBeamRenderer::Init(const Config& config)
		{
			m_config = config;
			m_isInited = true;
		}


		void LightBeamRenderer::Update(const stage::Stage& stage)
		{
			m_beamCount = 0;
			if (!m_isInited)
			{
				return;
			}

			const Config& c = m_config;
			const float unit = math::Max(c.modelSize, 0.01f);
			for (const stage::LightSegment& segment : stage.GetLightResult().segments)
			{
				const math::Vec2 delta = segment.end - segment.start;
				const float length = delta.Length();
				if (length < MIN_BEAM_LENGTH)
				{
					continue;
				}
				// 太さは光源の isWideBeam で決まる（0 なら細い光）
				const float width = segment.width > 0.0f ? segment.width : math::Max(c.thinBeamWidth, 0.01f);
				const math::Vec2 direction = delta * (1.0f / length);

				// 鏡で反射する端は、鏡の面に沿って切る：箱をその分だけ伸ばして、はみ出した所はシェーダーで捨てる
				// （光源の所と、壁で止まった所は、まっすぐな端）
				const bool hasStartCap = segment.startNormal.LengthSq() > 0.0001f;
				const bool hasEndCap = segment.endNormal.LengthSq() > 0.0001f;
				const float startExtension = hasStartCap ? CalcCapExtension(direction, math::Normalize(segment.startNormal), width * 0.5f) : 0.0f;
				const float endExtension = hasEndCap ? CalcCapExtension(direction, math::Normalize(segment.endNormal), width * 0.5f) : 0.0f;
				const math::Vec2 boxStart = segment.start - direction * startExtension;
				const math::Vec2 boxEnd = segment.end + direction * endExtension;
				const float boxLength = length + startExtension + endExtension;

				BeamModel& beam = AcquireBeam();
				Set4(beam.constants.color, c.color[0], c.color[1], c.color[2], c.color[3]);
				Set4(beam.constants.param, math::Clamp01(c.frontOpacity), math::Max(c.edgeGlow, 0.0f), 0.0f, 0.0f);
				SetCap(beam.constants.startCap, segment.startNormal, segment.start);
				SetCap(beam.constants.endCap, segment.endNormal, segment.end);

				Quaternion rotation;
				rotation.SetRotationDegZ(math::RadToDeg(std::atan2(delta.y, delta.x)));
				const math::Vec2 center = (boxStart + boxEnd) * 0.5f;
				beam.model.UpdateWorldMatrix(
					Vector3(center.x, center.y, 0.0f),
					rotation,
					Vector3(boxLength / unit, width / unit, width / unit));
			}
		}


		void LightBeamRenderer::Draw(RenderContext& rc)
		{
			(void)rc;
			// エンジンは登録した順に描く。半透明なので最後に登録する
			// 光は1本ずつ自分の定数バッファを持っているので、ライトの値（カメラの位置など）もそれぞれに写す
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			for (size_t i = 0; i < m_beamCount; ++i)
			{
				m_beams[i]->constants.light = light;
				RenderingEngine::GetInstance()->AddRenderObject(m_beams[i]->model);
			}
		}


		LightBeamRenderer::BeamModel& LightBeamRenderer::AcquireBeam()
		{
			// Model の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す
			if (m_beamCount >= m_beams.size())
			{
				auto beam = std::make_unique<BeamModel>();
				beam->constants = Constants();

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
				initData.m_expandConstantBuffer = &beam->constants;
				initData.m_expandConstantBufferSize = sizeof(Constants);
				// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
				initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
				// 半透明：後ろの絵と混ぜる。深度は書かない
				initData.m_alphaBlendMode = AlphaBlendMode_Trans;
				initData.m_isDepthWrite = false;
				initData.m_isDepthTest = true;
				beam->model.Init(initData);

				m_beams.push_back(std::move(beam));
			}
			return *m_beams[m_beamCount++];
		}
	}
}
