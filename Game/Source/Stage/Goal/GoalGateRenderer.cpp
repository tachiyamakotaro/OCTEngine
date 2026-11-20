/**
 * @file GoalGateRenderer.cpp
 * @brief ゴールの門を描く
 */
#include "stdafx.h"
#include "GoalGateRenderer.h"
#include "ClearSequence.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Stage/Stage.h"
#include <cmath>


namespace app
{
	namespace stage
	{
		namespace
		{
			// ---- モデル（Assets/modelData/object/。Tools/gen_goal_models.py で作った）----
			const char* const GATE_MODEL_PATH = "Assets/modelData/object/goal_gate.tkm";
			const char* const DOOR_LEFT_MODEL_PATH = "Assets/modelData/object/goal_door_l.tkm";
			const char* const DOOR_RIGHT_MODEL_PATH = "Assets/modelData/object/goal_door_r.tkm";
			const char* const LIGHT_MODEL_PATH = "Assets/modelData/object/goal_light.tkm";
			const char* const FLAME_MODEL_PATH = "Assets/modelData/object/goal_flame.tkm";
			// ---- 光る物のシェーダー ----
			const char* const GLOW_SHADER_PATH = "Assets/shader/goalGlow.fx";
			const char* const LIGHT_PS_ENTRY = "PSMainGoalLight";
			const char* const FLAME_PS_ENTRY = "PSMainGoalFlame";

			/**
			 * @brief 門の形の値（gen_goal_models.py と同じ値。モデルを作り直して形を変えたら、ここも合わせる）
			 * @details 座標は門の足元（ゴールの矩形の下端の真ん中）から
			 */
			namespace shape
			{
				/** 扉のちょうつがい（扉の口の端・扉の厚さの真ん中） */
				constexpr float DOOR_HINGE_X = 85.0f;
				constexpr float DOOR_HINGE_Z = 60.0f;
				/** 松明の皿の上の真ん中（炎の根元） */
				constexpr float TORCH_X = 105.0f;
				constexpr float TORCH_TOP_Y = 162.0f;
				constexpr float TORCH_Z = 13.0f;
			}

			/** 左右の炎のゆらめきをずらす角度 */
			constexpr float FLAME_PHASE_OFFSET = 2.3f;
		}


		void GoalGateRenderer::GlowModel::SetColor(const GlowColor& color, const GlowColor& core)
		{
			constants.glow.color = color.ToShaderValue();
			constants.glow.coreColor = core.ToShaderValue();
		}


		void GoalGateRenderer::GlowModel::Place(const Vector3& position, const Vector3& scale)
		{
			model.UpdateWorldMatrix(position, Quaternion::Identity, scale);
		}


		void GoalGateRenderer::Init(const GateStyle& style)
		{
			m_style = style;
			m_initialStyle = style;
			m_gates.clear();
			m_count = 0;
			m_time = 0.0f;
			m_isInited = true;
		}


		void GoalGateRenderer::Update(const stage::Stage& stage, const ClearSequence& sequence, const float deltaTime)
		{
			if (!m_isInited)
			{
				return;
			}
			m_time += deltaTime;
			m_count = 0;
			for (const stage::Goal& goal : stage.GetGimmicks().View<stage::Goal>())
			{
				const float openRate = math::Max(sequence.GetDoorOpenRate(goal), math::Clamp01(m_style.previewOpenRate));
				const Vector3 foot = GetFoot(goal);
				Gate& gate = Acquire();
				PlaceGate(gate, foot, openRate);
				PlaceLight(gate, foot, openRate);
				PlaceFlames(gate, foot, static_cast<float>(m_count));
			}
		}


		void GoalGateRenderer::Draw(RenderContext& rc)
		{
			// 光る物は自分の定数バッファを持っているので、ライトの値（カメラの位置など）もそれぞれに写す
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			RenderingEngine* renderingEngine = RenderingEngine::GetInstance();
			for (size_t i = 0; i < m_count; ++i)
			{
				Gate& gate = *m_gates[i];
				gate.frame.Draw(rc);
				gate.doorLeft.Draw(rc);
				gate.doorRight.Draw(rc);
				gate.light.constants.light = light;
				renderingEngine->AddRenderObject(gate.light.model);
				if (m_style.isTorchLit)
				{
					gate.flameLeft.constants.light = light;
					gate.flameRight.constants.light = light;
					renderingEngine->AddRenderObject(gate.flameLeft.model);
					renderingEngine->AddRenderObject(gate.flameRight.model);
				}
			}
		}


		GoalGateRenderer::Gate& GoalGateRenderer::Acquire()
		{
			// モデルの読み込みは重いので、足りなくなったときだけ増やし、毎フレーム使い回す
			if (m_count >= m_gates.size())
			{
				auto gate = std::make_unique<Gate>();
				// 門と扉はほかのギミック（GimmickRenderer）と同じ設定（影を受ける・Y-up）
				auto initModel = [](ModelRender& model, const char* path)
				{
					model.Init(
						path, nullptr, 0,
						false, true,
						&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
						enModelUpAxisY,
						D3D12_CULL_MODE_BACK);
				};
				initModel(gate->frame, GATE_MODEL_PATH);
				initModel(gate->doorLeft, DOOR_LEFT_MODEL_PATH);
				initModel(gate->doorRight, DOOR_RIGHT_MODEL_PATH);
				InitGlowModel(gate->light, LIGHT_MODEL_PATH, LIGHT_PS_ENTRY);
				InitGlowModel(gate->flameLeft, FLAME_MODEL_PATH, FLAME_PS_ENTRY);
				InitGlowModel(gate->flameRight, FLAME_MODEL_PATH, FLAME_PS_ENTRY);
				m_gates.push_back(std::move(gate));
			}
			return *m_gates[m_count++];
		}


		void GoalGateRenderer::InitGlowModel(GlowModel& glow, const char* tkmFilePath, const char* psEntry)
		{
			ModelInitData initData;
			initData.m_tkmFilePath = tkmFilePath;
			initData.m_fxFilePath = GLOW_SHADER_PATH;
			initData.m_vsEntryPointFunc = "VSMain";
			initData.m_vsSkinEntryPointFunc = "VSMain";
			initData.m_psEntryPointFunc = psEntry;
			initData.m_modelUpAxis = enModelUpAxisY;
			initData.m_cullMode = D3D12_CULL_MODE_BACK;
			// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
			initData.m_expandConstantBuffer = &glow.constants;
			initData.m_expandConstantBufferSize = sizeof(GlowConstants);
			// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
			initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
			glow.model.Init(initData);
		}


		void GoalGateRenderer::PlaceGate(Gate& gate, const Vector3& foot, const float openRate)
		{
			// 門の形の値（ちょうつがいの位置など）も、門の大きさに合わせる
			const float s = m_style.scale;
			gate.frame.SetPosition(foot);
			gate.frame.SetScale(GetScale());
			gate.frame.Update();

			// 扉は奥（+Z）へ開く。Y 軸まわりの回転は、+ で +X が -Z 側へ回る（DirectXMath の左手系）ので、
			// 右に伸びている左の扉は - に、左に伸びている右の扉は + に回す
			const float angle = m_style.doorOpenAngleDeg * openRate;
			Quaternion leftRotation;
			leftRotation.SetRotationDegY(-angle);
			Quaternion rightRotation;
			rightRotation.SetRotationDegY(angle);

			gate.doorLeft.SetPosition(foot + Vector3(-shape::DOOR_HINGE_X * s, 0.0f, shape::DOOR_HINGE_Z * s));
			gate.doorLeft.SetRotation(leftRotation);
			gate.doorLeft.SetScale(GetScale());
			gate.doorLeft.Update();
			gate.doorRight.SetPosition(foot + Vector3(shape::DOOR_HINGE_X * s, 0.0f, shape::DOOR_HINGE_Z * s));
			gate.doorRight.SetRotation(rightRotation);
			gate.doorRight.SetScale(GetScale());
			gate.doorRight.Update();
		}


		void GoalGateRenderer::PlaceLight(Gate& gate, const Vector3& foot, const float openRate)
		{
			// 閉じているときは合わせ目のすき間から見えるだけ。開くほど明るく、真ん中が白っぽくなる
			const GlowColor color = GlowColor::Lerp(m_style.lightClosed, m_style.lightOpen, openRate);
			const GlowColor core = GlowColor::Lerp(m_style.lightClosed, m_style.lightCore, openRate);
			gate.light.SetColor(color, core);
			gate.light.Place(foot, GetScale());
		}


		void GoalGateRenderer::PlaceFlames(Gate& gate, const Vector3& foot, const float seed)
		{
			gate.flameLeft.SetColor(m_style.flameOuter, m_style.flameInner);
			gate.flameRight.SetColor(m_style.flameOuter, m_style.flameInner);
			const float s = m_style.scale;
			gate.flameLeft.Place(foot + Vector3(-shape::TORCH_X * s, shape::TORCH_TOP_Y * s, shape::TORCH_Z * s), CalcFlameScale(seed));
			gate.flameRight.Place(foot + Vector3(shape::TORCH_X * s, shape::TORCH_TOP_Y * s, shape::TORCH_Z * s), CalcFlameScale(seed + FLAME_PHASE_OFFSET));
		}


		Vector3 GoalGateRenderer::CalcFlameScale(const float phase) const
		{
			// 速さの違う2つの波を足して、同じ動きのくり返しに見えないようにする
			const float t = m_time * m_style.flickerSpeed + phase;
			const float wave = std::sin(t) * 0.6f + std::sin(t * 2.3f + phase * 1.7f) * 0.4f;
			const float height = m_style.flameHeight * (1.0f + m_style.flickerAmount * wave);
			// 高くなったときは少し細く（炎が伸び縮みしているように見せる）
			const float width = m_style.flameWidth * (1.0f - m_style.flickerAmount * 0.5f * wave);
			// 炎のモデルは高さ 1・幅 1（門の大きさに合わせる）
			const float s = m_style.scale;
			return Vector3(width * s, height * s, width * s);
		}


		Vector3 GoalGateRenderer::GetFoot(const stage::Goal& goal) const
		{
			const math::Rect& rect = goal.rect;
			return Vector3((rect.min.x + rect.max.x) * 0.5f, rect.min.y, m_style.footZ);
		}


		Vector3 GoalGateRenderer::GetScale() const
		{
			return Vector3(m_style.scale, m_style.scale, m_style.scale);
		}
	}
}
