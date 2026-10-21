/**
 * @file StageRenderer.cpp
 * @brief ステージの地形をモデルで描く
 */
#include "stdafx.h"
#include "StageRenderer.h"
#include "Stage.h"
#include "Source/Render/MirrorReflection.h"


namespace app
{
	namespace stage
	{
		void StageRenderer::Init(const Config& config)
		{
			m_config = config;
			m_blocks = ModelGroup();
			m_movingBlocks = ModelGroup();
			m_panels.clear();
			m_panelCount = 0;
			m_isInited = true;
		}


		void StageRenderer::Update(const Stage& stage)
		{
			if (!m_isInited)
			{
				return;
			}

			// 地形ブロック
			m_blocks.rects.clear();
			for (const StageBlock& block : stage.GetGimmicks().View<StageBlock>())
			{
				m_blocks.rects.push_back(block.rect);
			}
			// 動く床・動く壁（今の位置。どれが動くのかわかるように、別のモデルで描く）
			m_movingBlocks.rects.clear();
			for (const MovingBlock& block : stage.GetGimmicks().View<MovingBlock>())
			{
				m_movingBlocks.rects.push_back(block.GetCurrentRect());
			}

			const BackMirror& mirror = stage.GetBackMirror();
			UpdateGroup(m_blocks, m_config.modelPath, mirror);
			UpdateGroup(m_movingBlocks, m_config.movingBlockModelPath, mirror);

			// 動く床・動く壁に付いたソーラーパネル（ためている間は黒 → 灰色 → 白、動いている間は白く光ったまま）
			m_panelCount = 0;
			for (const MovingBlock& block : stage.GetGimmicks().View<MovingBlock>())
			{
				PlacePanel(block.GetPanelRect(), block.GetPanelBrightness(), false);
				// 手前の面全体のパネル（見やすくするため。同じように光る）
				if (block.hasFrontPanel)
				{
					PlacePanel(block.GetCurrentRect(), block.GetPanelBrightness(), true);
				}
			}
		}


		void StageRenderer::Draw(RenderContext& rc)
		{
			DrawGroup(rc, m_blocks);
			DrawGroup(rc, m_movingBlocks);

			// パネルは1枚ずつ自分の定数バッファを持っているので、ライトの値（カメラの位置など）もそれぞれに写す
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			for (size_t i = 0; i < m_panelCount; ++i)
			{
				m_panels[i]->constants.light = light;
				RenderingEngine::GetInstance()->AddRenderObject(m_panels[i]->model);
			}
		}


		void StageRenderer::UpdateGroup(ModelGroup& group, const std::string& modelPath, const BackMirror& mirror)
		{
			const size_t count = group.rects.size();

			// 本物
			Reserve(group.models, count, D3D12_CULL_MODE_BACK, modelPath);
			for (size_t i = 0; i < count; ++i)
			{
				Place(*group.models[i], group.rects[i], false, 0.0f);
			}
			group.activeCount = count;

			// 鏡に映った像
			group.activeReflectionCount = 0;
			if (mirror.isEnabled)
			{
				Reserve(group.reflectionModels, count, D3D12_CULL_MODE_FRONT, modelPath);
				for (size_t i = 0; i < count; ++i)
				{
					Place(*group.reflectionModels[i], group.rects[i], true, mirror.planeZ);
				}
				group.activeReflectionCount = count;
			}
		}


		void StageRenderer::DrawGroup(RenderContext& rc, ModelGroup& group)
		{
			for (size_t i = 0; i < group.activeCount; ++i)
			{
				group.models[i]->Draw(rc);
			}
			for (size_t i = 0; i < group.activeReflectionCount; ++i)
			{
				group.reflectionModels[i]->Draw(rc);
			}
		}


		void StageRenderer::Place(ModelRender& model, const math::Rect& rect, const bool isReflected, const float planeZ)
		{
			Vector3 position;
			Vector3 scale;
			CalcTransform(rect, m_config.depth, position, scale);

			// 地形は回転しないので、鏡に映すのは位置と拡大率だけでよい
			if (isReflected)
			{
				position = render::ReflectPositionZ(position, planeZ);
				scale = render::ReflectScaleZ(scale);
			}
			model.SetPosition(position);
			model.SetScale(scale);
			model.Update();
		}


		void StageRenderer::CalcTransform(const math::Rect& rect, const float depth, Vector3& outPosition, Vector3& outScale) const
		{
			const Vector3& size = m_config.modelSize;
			const float centerX = (rect.min.x + rect.max.x) * 0.5f;
			if (m_config.isPlane)
			{
				// 板はブロックの上面に置く（モデルの原点は板の中心）
				outPosition = Vector3(centerX, rect.max.y, 0.0f);
				outScale = Vector3(rect.Width() / size.x, 1.0f, depth / size.z);
			}
			else
			{
				// 箱は幅・高さ・奥行きに拡大する
				// 原点が底面なら下端に、箱の中心なら矩形の中心に置く
				const float y = m_config.isPivotAtBottom ? rect.min.y : (rect.min.y + rect.max.y) * 0.5f;
				outPosition = Vector3(centerX, y, 0.0f);
				outScale = Vector3(rect.Width() / size.x, rect.Height() / size.y, depth / size.z);
			}
		}


		void StageRenderer::PlacePanel(const math::Rect& panelRect, const float brightness, const bool isFront)
		{
			// ブロックの面にぴったり重なるとちらつくので、パネルはブロックより少しだけ大きくする
			constexpr float PANEL_MARGIN = 1.0f;
			const math::Rect drawRect{
				{ panelRect.min.x - PANEL_MARGIN, panelRect.min.y - PANEL_MARGIN },
				{ panelRect.max.x + PANEL_MARGIN, panelRect.max.y + PANEL_MARGIN }
			};
			Vector3 position;
			Vector3 scale;
			if (isFront)
			{
				// 手前の面に貼る薄い板（厚さ PANEL_MARGIN * 2）。ブロックの手前の面（Z = -奥行きの半分）のすぐ手前に置く
				// （横の面の帯も手前に PANEL_MARGIN だけはみ出しているので、それよりさらに手前にして、重なってちらつかないようにする）
				CalcTransform(drawRect, PANEL_MARGIN * 2.0f, position, scale);
				position.z = -m_config.depth * 0.5f - PANEL_MARGIN;
			}
			else
			{
				// 面の帯を、奥行きいっぱいに置く
				CalcTransform(drawRect, m_config.depth + PANEL_MARGIN * 2.0f, position, scale);
			}

			// 色はシェーダーで混ぜる（セルの色：空のときの色 → 満タンのときの色。満タンに近いほど少し光る）
			PanelModel& panel = AcquirePanel();
			for (int i = 0; i < 3; ++i)
			{
				panel.constants.full[i] = m_config.panelFullColor[i];
				panel.constants.empty[i] = m_config.panelEmptyColor[i];
			}
			panel.constants.full[3] = math::Clamp01(brightness);
			panel.constants.empty[3] = m_config.panelGlow;
			// 模様の基準になる角（左下・手前）。ワールド座標のままだと、足場が動いても模様が止まったままになる
			panel.constants.origin[0] = drawRect.min.x;
			panel.constants.origin[1] = drawRect.min.y;
			panel.constants.origin[2] = 0.0f;
			panel.constants.origin[3] = 0.0f;
			panel.model.UpdateWorldMatrix(position, Quaternion::Identity, scale);
		}


		StageRenderer::PanelModel& StageRenderer::AcquirePanel()
		{
			// Model の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す
			if (m_panelCount >= m_panels.size())
			{
				auto panel = std::make_unique<PanelModel>();
				panel->constants = PanelConstants();

				ModelInitData initData;
				initData.m_tkmFilePath = m_config.solarPanelModelPath.c_str();
				initData.m_fxFilePath = "Assets/shader/solarPanel.fx";
				initData.m_vsEntryPointFunc = "VSMain";
				initData.m_vsSkinEntryPointFunc = "VSMain";
				initData.m_psEntryPointFunc = "PSMainSolarPanel";
				initData.m_modelUpAxis = m_config.upAxis;
				initData.m_cullMode = D3D12_CULL_MODE_BACK;
				// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
				initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
				// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
				initData.m_expandConstantBuffer = &panel->constants;
				initData.m_expandConstantBufferSize = sizeof(PanelConstants);
				// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
				initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
				panel->model.Init(initData);

				m_panels.push_back(std::move(panel));
			}
			return *m_panels[m_panelCount++];
		}


		void StageRenderer::Reserve(std::vector<std::unique_ptr<ModelRender>>& models, const size_t count, const D3D12_CULL_MODE cullMode, const std::string& modelPath)
		{
			// ModelRender の初期化は重いので、足りない分だけ作り、余った分は描かないだけにする
			while (models.size() < count)
			{
				auto model = std::make_unique<ModelRender>();
				model->Init(
					modelPath.c_str(), nullptr, 0,
					false, m_config.isShadowReceiver,
					&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
					m_config.upAxis,
					cullMode);
				models.push_back(std::move(model));
			}
		}
	}
}
