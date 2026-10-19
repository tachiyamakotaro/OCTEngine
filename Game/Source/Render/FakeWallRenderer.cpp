/**
 * @file FakeWallRenderer.cpp
 * @brief 偽物の壁と、手鏡の視界の扇形を描く
 */
#include "stdafx.h"
#include "FakeWallRenderer.h"
#include <cmath>
#include "MirrorReflection.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/Gimmick/FakeWall.h"
#include "Source/Stage/Gimmick/HandMirror.h"
#include "Source/Stage/LightTracer.h"


static_assert(app::stage::HandMirror::MAX_REVEAL_COUNT <= 32, "fakeWall.fx の REVEAL_MAX（32）より多い");
static_assert(app::stage::FakeWall::MAX_LIGHT_HOLES <= 16, "fakeWall.fx の LIGHT_HOLE_MAX（16）より多い");


namespace
{
	/** シェーダー */
	const char* const SHADER_PATH = "Assets/shader/fakeWall.fx";
	/** 壁のピクセルシェーダー（視界の中を捨てる。影を受ける） */
	const char* const WALL_PS_ENTRY = "PSMainFakeWall";
	/** 扇形のピクセルシェーダー（視界の中だけ半透明で塗る） */
	const char* const FAN_PS_ENTRY = "PSMainMirrorFan";
	/** 光のピクセルシェーダー（半透明で、上下の縁を少し光らせる） */
	const char* const BEAM_PS_ENTRY = "PSMainLightBeam";
	/** これより短い光の線分は描かない（GimmickRenderer と同じ） */
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
	 * @param direction 光の向き（長さ1）
	 * @param normal 鏡の面の向き（長さ1）
	 * @param halfWidth 光の太さの半分
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
		FakeWallRenderer::FakeWallRenderer()
			: m_config()
			, m_constants()
			, m_walls()
			, m_wallCount(0)
			, m_reflectedWalls()
			, m_reflectedWallCount(0)
			, m_fan()
			, m_beams()
			, m_beamCount(0)
			, m_isFanDrawn(false)
			, m_isInited(false)
		{}


		void FakeWallRenderer::Init(const Config& config)
		{
			m_config = config;
			if (!m_isInited)
			{
				InitModel(m_fan, m_config.fanModelPath.c_str(), enModelUpAxisY, FAN_PS_ENTRY, D3D12_CULL_MODE_BACK, true);
			}
			m_isInited = true;
		}


		void FakeWallRenderer::Update(const stage::Stage& stage)
		{
			m_wallCount = 0;
			m_reflectedWallCount = 0;
			m_beamCount = 0;
			m_isFanDrawn = false;
			if (!m_isInited)
			{
				return;
			}

			// ---- 手鏡の視界（構えている手鏡があれば、シェーダーに渡す）----
			const stage::HandMirror* activeMirror = nullptr;
			for (const stage::HandMirror& mirror : stage.GetGimmicks().View<stage::HandMirror>())
			{
				if (mirror.IsViewActive())
				{
					activeMirror = &mirror;
					break;
				}
			}
			stage::HandMirrorCone cone;
			if (activeMirror != nullptr)
			{
				cone = activeMirror->GetCone();
				const float halfRad = math::DegToRad(cone.halfAngleDeg);
				Set4(m_constants.cone, cone.origin.x, cone.origin.y, cone.direction.x, cone.direction.y);
				Set4(m_constants.coneParam, std::sin(halfRad), std::cos(halfRad), cone.range, 1.0f);
			}
			else
			{
				// w = 0 で「構えていない」（壁はどこも消えない）
				Set4(m_constants.coneParam, 0.0f, 1.0f, 0.0f, 0.0f);
			}
			const Config& c = m_config;
			Set4(m_constants.glow, c.edgeGlowColor[0], c.edgeGlowColor[1], c.edgeGlowColor[2], math::Max(c.edgeGlowWidth, 0.0f));
			Set4(m_constants.fanColor, c.fanColor[0], c.fanColor[1], c.fanColor[2], c.fanColor[3]);
			Set4(m_constants.fanEdgeColor, c.fanEdgeColor[0], c.fanEdgeColor[1], c.fanEdgeColor[2], c.fanEdgeColor[3]);
			Set4(m_constants.fanParam, math::Max(c.fanEdgeWidth, 0.0f), math::Max(c.fanDashLength, 0.0f), 0.0f, 0.0f);

			// ---- 偽物の壁が消えている範囲（今の視界と、少し前に当てていた所。FakeWall の当たり判定と同じ物）----
			m_reveals.clear();
			for (const stage::HandMirror& mirror : stage.GetGimmicks().View<stage::HandMirror>())
			{
				mirror.CollectReveals(m_reveals);
			}
			const int revealCount = static_cast<int>(m_reveals.size()) < REVEAL_MAX ? static_cast<int>(m_reveals.size()) : REVEAL_MAX;
			for (int i = 0; i < revealCount; ++i)
			{
				const stage::HandMirrorCone& revealCone = m_reveals[i].cone;
				const float revealHalfRad = math::DegToRad(revealCone.halfAngleDeg);
				Set4(m_constants.revealCone[i], revealCone.origin.x, revealCone.origin.y, revealCone.direction.x, revealCone.direction.y);
				Set4(m_constants.revealParam[i], std::sin(revealHalfRad), std::cos(revealHalfRad), revealCone.range, m_reveals[i].rate);
			}
			Set4(m_constants.revealInfo, static_cast<float>(revealCount), math::Max(c.dissolveCellSize, 0.01f), math::Max(c.dissolveGlowWidth, 0.0f), 0.0f);

			// ---- 光で開いた穴（その壁の範囲も一緒に渡して、隣の偽物の壁まで消さないようにする）----
			int holeCount = 0;
			for (const stage::FakeWall& wall : stage.GetGimmicks().View<stage::FakeWall>())
			{
				for (const stage::FakeWallLightHole& hole : wall.lightHoles)
				{
					if (holeCount >= LIGHT_HOLE_MAX)
					{
						break;
					}
					Set4(m_constants.lightHoleSeg[holeCount], hole.a.x, hole.a.y, hole.b.x, hole.b.y);
					Set4(m_constants.lightHoleParam[holeCount], hole.radius, 0.0f, 0.0f, 0.0f);
					Set4(m_constants.lightHoleClip[holeCount], wall.rect.min.x, wall.rect.min.y, wall.rect.max.x, wall.rect.max.y);
					++holeCount;
				}
			}
			Set4(m_constants.lightHoleInfo, static_cast<float>(holeCount), 0.0f, 0.0f, 0.0f);

			// ---- 偽物の壁（本物と同じ置き方。奥の鏡があれば映った壁も）----
			const stage::BackMirror& backMirror = stage.GetBackMirror();
			const bool isReflect = backMirror.isEnabled && c.isDrawReflection;
			for (const stage::FakeWall& wall : stage.GetGimmicks().View<stage::FakeWall>())
			{
				// 反射した光で壁ごと消えている間は描かない
				if (wall.IsWhollyRevealed())
				{
					continue;
				}
				PlaceWall(AcquireWall(m_walls, m_wallCount, D3D12_CULL_MODE_BACK), wall.rect, false, 0.0f);
				if (isReflect)
				{
					PlaceWall(AcquireWall(m_reflectedWalls, m_reflectedWallCount, D3D12_CULL_MODE_FRONT), wall.rect, true, backMirror.planeZ);
				}
			}

			// ---- 光：線分ごとに、中点に置いて長さまで伸ばす（光源から出る光は全部ここで描く）----
			const float beamUnit = math::Max(c.beamModelSize, 0.01f);
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
				// （光源の所と、壁で止まった所は、今まで通りまっすぐな端）
				const bool hasStartCap = segment.startNormal.LengthSq() > 0.0001f;
				const bool hasEndCap = segment.endNormal.LengthSq() > 0.0001f;
				const float startExtension = hasStartCap ? CalcCapExtension(direction, math::Normalize(segment.startNormal), width * 0.5f) : 0.0f;
				const float endExtension = hasEndCap ? CalcCapExtension(direction, math::Normalize(segment.endNormal), width * 0.5f) : 0.0f;
				const math::Vec2 boxStart = segment.start - direction * startExtension;
				const math::Vec2 boxEnd = segment.end + direction * endExtension;
				const float boxLength = length + startExtension + endExtension;

				BeamModel& beam = AcquireBeam();
				Set4(beam.constants.beamColor, c.beamColor[0], c.beamColor[1], c.beamColor[2], c.beamColor[3]);
				Set4(beam.constants.beamParam, math::Clamp01(c.beamFrontOpacity), math::Max(c.beamEdgeGlow, 0.0f), 0.0f, 0.0f);
				SetCap(beam.constants.beamStartCap, segment.startNormal, segment.start);
				SetCap(beam.constants.beamEndCap, segment.endNormal, segment.end);

				Quaternion rotation;
				rotation.SetRotationDegZ(math::RadToDeg(std::atan2(delta.y, delta.x)));
				const math::Vec2 center = (boxStart + boxEnd) * 0.5f;
				beam.model.UpdateWorldMatrix(
					Vector3(center.x, center.y, 0.0f),
					rotation,
					Vector3(boxLength / beamUnit, width / beamUnit, width / beamUnit));
			}

			// ---- 視界の扇形：頂点を中心に、長さ×2 の正方形の板を置く（扇形の外はシェーダーが捨てる）----
			if (activeMirror != nullptr && c.isShowFan)
			{
				const float scale = cone.range * 2.0f / math::Max(c.fanModelSize, 0.01f);
				m_fan.UpdateWorldMatrix(
					Vector3(cone.origin.x, cone.origin.y, c.fanZ),
					Quaternion(),
					Vector3(scale, scale, 1.0f / math::Max(c.fanModelSize, 0.01f)));
				m_isFanDrawn = true;
			}
		}


		void FakeWallRenderer::DrawWalls(RenderContext& rc)
		{
			(void)rc;
			// ライトの値は描く直前に写す（カメラの位置などが、このフレームの値になるように）
			m_constants.light = SceneLight::GetInstance()->GetSceneLight();
			for (size_t i = 0; i < m_wallCount; ++i)
			{
				RenderingEngine::GetInstance()->AddRenderObject(*m_walls[i]);
			}
			for (size_t i = 0; i < m_reflectedWallCount; ++i)
			{
				RenderingEngine::GetInstance()->AddRenderObject(*m_reflectedWalls[i]);
			}
		}


		void FakeWallRenderer::DrawFan(RenderContext& rc)
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
			if (m_isFanDrawn)
			{
				RenderingEngine::GetInstance()->AddRenderObject(m_fan);
			}
		}


		void FakeWallRenderer::InitModel(
			Model& model, const char* tkmFilePath, const EnModelUpAxis upAxis,
			const char* psEntryPoint, const D3D12_CULL_MODE cullMode, const bool isTranslucent, Constants* constants)
		{
			// 定数バッファを指定しなければ、壁と扇形で共有の m_constants を使う
			Constants* const target = constants != nullptr ? constants : &m_constants;
			ModelInitData initData;
			initData.m_tkmFilePath = tkmFilePath;
			initData.m_fxFilePath = SHADER_PATH;
			initData.m_vsEntryPointFunc = "VSMain";
			initData.m_vsSkinEntryPointFunc = "VSMain";
			initData.m_psEntryPointFunc = psEntryPoint;
			initData.m_modelUpAxis = upAxis;
			initData.m_cullMode = cullMode;
			// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
			initData.m_expandConstantBuffer = target;
			initData.m_expandConstantBufferSize = sizeof(Constants);
			// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
			initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
			if (isTranslucent)
			{
				// 半透明：後ろの絵と混ぜる。深度は書かない
				initData.m_alphaBlendMode = AlphaBlendMode_Trans;
				initData.m_isDepthWrite = false;
				initData.m_isDepthTest = true;
			}
			model.Init(initData);
		}


		Model& FakeWallRenderer::AcquireWall(std::vector<std::unique_ptr<Model>>& pool, size_t& usedCount, const D3D12_CULL_MODE cullMode)
		{
			// Model の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す
			if (usedCount >= pool.size())
			{
				auto model = std::make_unique<Model>();
				InitModel(*model, m_config.wallModelPath.c_str(), m_config.wallUpAxis, WALL_PS_ENTRY, cullMode, false);
				pool.push_back(std::move(model));
			}
			return *pool[usedCount++];
		}


		FakeWallRenderer::BeamModel& FakeWallRenderer::AcquireBeam()
		{
			// 壁と同じく、足りなくなったときだけ増やして使い回す
			// （unique_ptr で持つので、vector が大きくなっても constants の場所は変わらない。Model はこの場所を覚えている）
			if (m_beamCount >= m_beams.size())
			{
				auto beam = std::make_unique<BeamModel>();
				beam->constants = Constants();
				InitModel(beam->model, m_config.beamModelPath.c_str(), enModelUpAxisY, BEAM_PS_ENTRY, D3D12_CULL_MODE_BACK, true, &beam->constants);
				m_beams.push_back(std::move(beam));
			}
			return *m_beams[m_beamCount++];
		}


		void FakeWallRenderer::PlaceWall(Model& model, const math::Rect& rect, const bool isReflected, const float planeZ)
		{
			const Vector3& size = m_config.wallModelSize;
			const float centerX = (rect.min.x + rect.max.x) * 0.5f;
			const float y = m_config.isWallPivotAtBottom ? rect.min.y : (rect.min.y + rect.max.y) * 0.5f;
			Vector3 position(centerX, y, 0.0f);
			Vector3 scale(rect.Width() / size.x, rect.Height() / size.y, m_config.wallDepth / size.z);

			// 鏡に映すのは位置と拡大率だけでよい（回転しないので）
			if (isReflected)
			{
				position = ReflectPositionZ(position, planeZ);
				scale = ReflectScaleZ(scale);
			}
			model.UpdateWorldMatrix(position, Quaternion(), scale);
		}
	}
}
