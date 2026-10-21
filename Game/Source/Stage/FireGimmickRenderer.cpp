/**
 * @file FireGimmickRenderer.cpp
 * @brief 虫眼鏡のギミックと、集まる光・焦点・炎を描く
 */
#include "stdafx.h"
#include "FireGimmickRenderer.h"
#include <cmath>
#include "Source/Core/Math/MathUtil.h"
#include "Stage.h"


namespace
{
	/** 半透明のモデルのシェーダー（光と同じ） */
	const char* const GLOW_SHADER_PATH = "Assets/shader/lightBeam.fx";
	const char* const GLOW_PS_ENTRY = "PSMainLightBeam";


	void Set4(float out[4], const float x, const float y, const float z, const float w)
	{
		out[0] = x;
		out[1] = y;
		out[2] = z;
		out[3] = w;
	}


	/** 炎ごとにゆらぎをずらすための値（0〜2π） */
	float MakePhase(const int a, const int b)
	{
		const float s = std::sin(static_cast<float>(a) * 12.9898f + static_cast<float>(b) * 78.233f) * 43758.5453f;
		return (s - std::floor(s)) * 6.2831853f;
	}
}


namespace app
{
	namespace stage
	{
		ModelRender& FireGimmickRenderer::ModelPool::Acquire()
		{
			// ModelRender の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す（GimmickRenderer と同じ）
			if (usedCount >= models.size())
			{
				auto model = std::make_unique<ModelRender>();
				model->Init(
					path.c_str(), nullptr, 0,
					false, true,
					&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
					enModelUpAxisY,
					D3D12_CULL_MODE_BACK);
				models.push_back(std::move(model));
			}
			return *models[usedCount++];
		}


		FireGimmickRenderer::GlowModel& FireGimmickRenderer::GlowPool::Acquire()
		{
			// LightBeamRenderer::AcquireBeam と同じ作り方（半透明・深度は書かない）
			if (usedCount >= models.size())
			{
				auto glow = std::make_unique<GlowModel>();
				glow->constants = Constants();

				ModelInitData initData;
				initData.m_tkmFilePath = path.c_str();
				initData.m_fxFilePath = GLOW_SHADER_PATH;
				initData.m_vsEntryPointFunc = "VSMain";
				initData.m_vsSkinEntryPointFunc = "VSMain";
				initData.m_psEntryPointFunc = GLOW_PS_ENTRY;
				initData.m_modelUpAxis = enModelUpAxisY;
				initData.m_cullMode = D3D12_CULL_MODE_BACK;
				initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
				initData.m_expandConstantBuffer = &glow->constants;
				initData.m_expandConstantBufferSize = sizeof(Constants);
				initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
				initData.m_alphaBlendMode = AlphaBlendMode_Trans;
				initData.m_isDepthWrite = false;
				initData.m_isDepthTest = true;
				glow->model.Init(initData);

				models.push_back(std::move(glow));
			}
			return *models[usedCount++];
		}


		void FireGimmickRenderer::Init(const Config& config)
		{
			m_config = config;
			for (int i = 0; i < static_cast<int>(ModelKind::Num); ++i)
			{
				m_pools[i] = ModelPool();
				m_pools[i].path = config.modelPaths[i];
			}
			for (int i = 0; i < static_cast<int>(GlowKind::Num); ++i)
			{
				m_glowPools[i].models.clear();
				m_glowPools[i].usedCount = 0;
				m_glowPools[i].path = config.glowModelPaths[i];
			}
			m_time = 0.0f;
			m_isInited = true;
		}


		void FireGimmickRenderer::Update(const Stage& stage, const float deltaTime)
		{
			if (!m_isInited)
			{
				return;
			}
			for (ModelPool& pool : m_pools) { pool.usedCount = 0; }
			for (GlowPool& pool : m_glowPools) { pool.usedCount = 0; }
			m_flameCount = 0;
			m_time += deltaTime;

			const Config& c = m_config;
			const GimmickManager& gimmicks = stage.GetGimmicks();

			// 虫眼鏡と、集まる光
			for (const Magnifier& magnifier : gimmicks.View<Magnifier>())
			{
				PutMagnifier(magnifier);
			}

			// 草むら：燃えている間は縮んだ形を、こげた色で
			for (const Grass& grass : gimmicks.View<Grass>())
			{
				if (!grass.isBurnt)
				{
					PutRect(grass.isBurning ? ModelKind::GrassBurning : ModelKind::Grass, grass.GetCurrentRect(), c.grassDepth);
				}
			}

			// ロープ：燃え尽きたら消える
			for (const Rope& rope : gimmicks.View<Rope>())
			{
				if (!rope.isBurnt)
				{
					PutRect(rope.isBurning ? ModelKind::RopeBurning : ModelKind::Rope, rope.GetCurrentRect(), c.ropeDepth);
				}
			}

			// 吊るされた板：今の位置に
			for (const HangingBlock& block : gimmicks.View<HangingBlock>())
			{
				PutRect(ModelKind::Plank, block.GetCurrentRect(), c.plankDepth);
			}

			// 炎：物の手前の面に出す（草は奥行きがあるので、その手前）
			int objectIndex = 0;
			for (const BurnableGimmick& burnable : gimmicks.View<BurnableGimmick>())
			{
				const bool isRope = dynamic_cast<const Rope*>(&burnable) != nullptr;
				const float frontZ = -(isRope ? c.ropeDepth : c.grassDepth) * 0.5f - 5.0f;
				PutFlames(burnable, isRope, frontZ, objectIndex++);
			}
		}


		void FireGimmickRenderer::DrawOpaque(RenderContext& rc)
		{
			for (ModelPool& pool : m_pools)
			{
				for (size_t i = 0; i < pool.usedCount; ++i)
				{
					pool.models[i]->Draw(rc);
				}
			}
		}


		void FireGimmickRenderer::DrawTranslucent(RenderContext& rc)
		{
			(void)rc;
			// エンジンは登録した順に描く。半透明なので最後のほうに登録する。ライトの値（カメラの位置など）はそれぞれに写す
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			for (GlowPool& pool : m_glowPools)
			{
				for (size_t i = 0; i < pool.usedCount; ++i)
				{
					pool.models[i]->constants.light = light;
					RenderingEngine::GetInstance()->AddRenderObject(pool.models[i]->model);
				}
			}
		}


		void FireGimmickRenderer::Put(const ModelKind kind, const math::Vec2& center, const float z, const float angleDeg, const Vector3& size)
		{
			const float unit = m_config.modelSize;
			Quaternion rotation;
			rotation.SetRotationDegZ(angleDeg);
			ModelRender& model = m_pools[static_cast<int>(kind)].Acquire();
			model.SetPosition(Vector3(center.x, center.y, z));
			model.SetRotation(rotation);
			model.SetScale(Vector3(size.x / unit, size.y / unit, size.z / unit));
			model.Update();
		}


		void FireGimmickRenderer::PutRect(const ModelKind kind, const math::Rect& rect, const float depth)
		{
			if (rect.Width() <= 0.01f || rect.Height() <= 0.01f)
			{
				return;
			}
			Put(kind, (rect.min + rect.max) * 0.5f, 0.0f, 0.0f, Vector3(rect.Width(), rect.Height(), depth));
		}


		void FireGimmickRenderer::PutGlow(
			const GlowKind kind, const GlowStyle& style, const Vector3& position, const float angleDeg, const Vector3& size,
			const math::Vec2& endCapNormal, const math::Vec2& endCapPoint)
		{
			const float unit = m_config.modelSize;
			GlowModel& glow = m_glowPools[static_cast<int>(kind)].Acquire();
			Set4(glow.constants.color, style.color[0], style.color[1], style.color[2], math::Clamp01(style.color[3]));
			Set4(glow.constants.param, math::Clamp01(style.frontOpacity), math::Max(style.edgeGlow, 0.0f), 0.0f, 0.0f);
			Set4(glow.constants.startCap, 0.0f, 0.0f, 0.0f, 0.0f);
			if (endCapNormal.LengthSq() > 0.0001f)
			{
				const math::Vec2 n = math::Normalize(endCapNormal);
				Set4(glow.constants.endCap, n.x, n.y, endCapPoint.x, endCapPoint.y);
			}
			else
			{
				Set4(glow.constants.endCap, 0.0f, 0.0f, 0.0f, 0.0f);
			}

			Quaternion rotation;
			rotation.SetRotationDegZ(angleDeg);
			glow.model.UpdateWorldMatrix(position, rotation, Vector3(size.x / unit, size.y / unit, size.z / unit));
		}


		void FireGimmickRenderer::PutMagnifier(const Magnifier& magnifier)
		{
			const Config& c = m_config;
			const float diameter = magnifier.diameter;

			// 虫眼鏡のモデル：+X が光を通す向き。光が当たっている間はガラスが温かい色
			Put(magnifier.isLit ? ModelKind::MagnifierLit : ModelKind::Magnifier,
				magnifier.center, 0.0f, magnifier.angleDeg, Vector3(diameter, diameter, diameter));

			if (!magnifier.isLit && !magnifier.isShowGuide)
			{
				return;
			}

			// 集まる光の円すい：底をレンズの中心、先を焦点に合わせる。
			// 焦点より手前で何かに止められたら、そこから先を切る（光はその物の面までしか届かない）
			const math::Vec2 direction = magnifier.focusDirection;
			const float focalLength = math::Max(magnifier.focalLength, 1.0f);
			const float width = diameter * c.coneWidthRate;
			const math::Vec2 coneCenter = magnifier.center + direction * (focalLength * 0.5f);
			const float angleDeg = math::RadToDeg(std::atan2(direction.y, direction.x));
			const bool isCut = magnifier.coneLength < focalLength;
			const math::Vec2 capNormal = isCut ? direction * -1.0f : math::Vec2::Zero();
			const math::Vec2 focusPoint = magnifier.GetFocusPoint();

			GlowStyle coneStyle = magnifier.isLit ? c.litCone : c.guideCone;
			if (!magnifier.isLit)
			{
				// 目印はゆっくり明るくなったり暗くなったりして、目に留まるようにする
				coneStyle.color[3] += c.guidePulseAmount * (0.5f + 0.5f * std::sin(m_time * c.guidePulseSpeed));
			}
			PutGlow(GlowKind::Cone, coneStyle, Vector3(coneCenter.x, coneCenter.y, 0.0f), angleDeg,
				Vector3(focalLength, width, width), capNormal, focusPoint);

			// 焦点の光の玉：燃やせるときは明るく脈打つ。さえぎられているときはぼやけて広がる。目印のときは小さく
			const Vector3 spotPosition(focusPoint.x, focusPoint.y, 0.0f);
			if (magnifier.CanBurn())
			{
				const float size = c.hotSpotSize * (1.0f + 0.15f * std::sin(m_time * 12.0f));
				PutGlow(GlowKind::Spot, c.hotSpot, spotPosition, 0.0f, Vector3(size, size, size));
			}
			else if (magnifier.isLit)
			{
				const float size = c.blurSpotSize;
				PutGlow(GlowKind::Spot, c.blurSpot, spotPosition, 0.0f, Vector3(size, size, size));
			}
			else
			{
				GlowStyle style = c.guideSpot;
				style.color[3] += c.guidePulseAmount * (0.5f + 0.5f * std::sin(m_time * c.guidePulseSpeed));
				const float size = c.guideSpotSize;
				PutGlow(GlowKind::Spot, style, spotPosition, 0.0f, Vector3(size, size, size));
			}
		}


		void FireGimmickRenderer::PutFlames(const BurnableGimmick& burnable, const bool isRope, const float frontZ, const int objectIndex)
		{
			const Config& c = m_config;
			if (burnable.isBurnt)
			{
				return;
			}

			// 火がつく前：温まった点に小さな炎がくすぶる（もうすぐ燃えることがわかる）
			if (!burnable.isBurning)
			{
				if (burnable.heat >= c.smolderStartHeat)
				{
					const float t = (burnable.heat - c.smolderStartHeat) / math::Max(1.0f - c.smolderStartHeat, 0.01f);
					PutFlame(burnable.heatPoint, frontZ, c.smolderFlameSize * (0.3f + 0.7f * t), MakePhase(objectIndex, 0));
				}
				return;
			}

			// 燃えている：今の形の中に、炎を格子に並べる（燃え尽きる直前は小さくなる）
			const math::Rect rect = burnable.GetCurrentRect();
			const float progress = burnable.GetBurnProgress();
			const float baseSize = (isRope ? c.ropeFlameSize : c.grassFlameSize) * (1.0f - 0.5f * progress * progress);
			const float pitch = baseSize * 0.8f;
			int columns = static_cast<int>(math::Max(1.0f, std::round(rect.Width() / pitch)));
			int rows = static_cast<int>(math::Max(1.0f, std::round(rect.Height() / pitch)));
			// 多すぎるときは間隔を広げる
			while (columns * rows > c.maxFlamesPerObject)
			{
				if (columns >= rows) { columns = (columns + 1) / 2; }
				else { rows = (rows + 1) / 2; }
			}
			for (int row = 0; row < rows; ++row)
			{
				for (int column = 0; column < columns; ++column)
				{
					const float x = rect.min.x + rect.Width() * (static_cast<float>(column) + 0.5f) / static_cast<float>(columns);
					// 炎の根元は、各マスの下のほう（炎は上に伸びる）
					const float y = rect.min.y + rect.Height() * (static_cast<float>(row) + 0.2f) / static_cast<float>(rows);
					PutFlame({ x, y }, frontZ, baseSize, MakePhase(objectIndex, row * 31 + column));
				}
			}
		}


		void FireGimmickRenderer::PutFlame(const math::Vec2& position, const float z, const float size, const float phase)
		{
			const Config& c = m_config;
			if (m_flameCount >= c.maxFlames || size <= 0.5f)
			{
				return;
			}
			++m_flameCount;

			// ゆらぎ：高さは大きく、幅は少しだけ。速さも炎ごとに少し変える
			const float height = size * (0.85f + 0.25f * std::sin(m_time * 11.0f + phase));
			const float width = size * 0.7f * (0.92f + 0.08f * std::sin(m_time * 7.0f + phase * 1.7f));
			const float sway = 4.0f * std::sin(m_time * 5.0f + phase);
			// モデルは原点が真ん中なので、根元が position に来るように半分上げる
			const Vector3 center(position.x, position.y + height * 0.5f, z);
			PutGlow(GlowKind::Flame, c.flameOuter, center, sway, Vector3(width, height, width));

			const float inner = c.innerFlameRate;
			const Vector3 innerCenter(position.x, position.y + height * inner * 0.5f, z - 2.0f);
			PutGlow(GlowKind::Flame, c.flameInner, innerCenter, sway, Vector3(width * inner, height * inner, width * inner));
		}
	}
}
