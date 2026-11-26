/**
 * @file RuinEntrance.cpp
 * @brief タイトル画面の遺跡の入口
 */
#include "stdafx.h"
#include "RuinEntrance.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace title
	{
		namespace
		{
			/** 門の光に使うシーンのポイントライトの番号（宝物の間の宝石の光と同じ 0 番。同時には使わない） */
			constexpr int GATE_LIGHT_INDEX = 0;

			/** 地面の奥行き（Z = -300〜300）と、建物の奥行き（Z = -20〜20） */
			constexpr float GROUND_DEPTH = 600.0f;
			constexpr float RUIN_DEPTH = 40.0f;

			/** 門のモデル（goal_gate.tkm）の奥行き（足元から奥の面まで。大きさ 1 のとき） */
			constexpr float GATE_MODEL_DEPTH = 150.0f;
			/** 門の奥の面と、壁の手前の面のすき間（ぴったりだとちらつくので少しあける） */
			constexpr float GATE_WALL_GAP = 2.0f;

			/**
			 * @brief 遺跡の建物の形（門の真ん中 gateX からの距離。地面の上が y = 0）
			 * @details 門は壁より手前に置くので、壁に口は空けない（門の後ろも壁でふさぐ）
			 *          門（大きさ 1.4 で幅 353・高さ 381）より高い壁にして、右へ段々に高くする
			 *
			 *                     ┌──────────── 3段目
			 *                ┌────┘
			 *            ┌───┘ 2段目
			 *            │   ┌──門──┐
			 *            │壁 │      │        → 画面の右の外まで
			 *     ───────┴───┴──────┴──────────── 地面
			 */
			namespace ruin_shape
			{
				/** 壁の左端（門の真ん中から）と高さ */
				constexpr float WALL_LEFT = -220.0f;
				constexpr float WALL_HEIGHT = 450.0f;
				/** 2段目・3段目の左端（門の真ん中から）と高さ */
				constexpr float TIER2_LEFT = -130.0f;
				constexpr float TIER2_HEIGHT = 90.0f;
				constexpr float TIER3_LEFT = -20.0f;
				constexpr float TIER3_HEIGHT = 500.0f;
				/** 建物の右端（門の真ん中から。画面の右の外まで伸ばす） */
				constexpr float RIGHT_END = 900.0f;
			}

			/**
			 * @brief 門の足元の段（goal_gate.tkm の形。大きさ 1 のとき。門の足元からの奥行きと、段の上の高さ）
			 * @details 1段目は奥行き 8 から高さ 7、2段目は奥行き 20 から高さ 13（門の中の床もこの高さ。扉の奥の光の板まで続く）
			 *          門のモデルを作り直して段の形を変えたら、ここも合わせる
			 */
			namespace gate_step
			{
				struct Step
				{
					/** 段の手前の面（門の足元からの奥行き） */
					float frontZ;
					/** 段の上の高さ */
					float topY;
				};
				constexpr Step STEPS[] =
				{
					{ 8.0f, 7.0f },
					{ 20.0f, 13.0f },
				};
				/** 段の手前の、上り始める所から段の手前の面までの長さ（ここで少しずつ上がる。上るアニメーションがないので、ぱっと上がって見えないように） */
				constexpr float STEP_UP_LENGTH = 12.0f;
			}

			/** 門のゴールの判定の大きさ（判定は使わない。門を置く位置を決めるだけ。下端の真ん中に門が立つ） */
			constexpr float GATE_GOAL_WIDTH = 150.0f;
			constexpr float GATE_GOAL_HEIGHT = 250.0f;

			/** 地形ブロックの描画の設定（宝物の間と同じ箱のモデル） */
			stage::StageRenderer::Config MakeRendererConfig(const float depth)
			{
				stage::StageRenderer::Config config;
				config.modelPath = "Assets/modelData/Box.tkm";
				config.upAxis = enModelUpAxisZ;
				config.modelSize = Vector3(100.0f, 100.0f, 100.0f);
				config.isPlane = false;
				config.isPivotAtBottom = true;
				config.depth = depth;
				return config;
			}
		}


		RuinEntrance::~RuinEntrance()
		{
			// シーンのライトはどのシーンでも同じ物なので、門の光（ポイントライト）を残さない
			ClearGateLight();
		}


		void RuinEntrance::Init(const RuinEntranceLayout& layout, const float doorOpenTime)
		{
			m_layout = layout;
			m_lightStyle = GateLightStyle();

			BuildGround();
			BuildRuin();
			m_groundRenderer.Init(MakeRendererConfig(GROUND_DEPTH));
			m_ruinRenderer.Init(MakeRendererConfig(RUIN_DEPTH));
			// 門は大きくして、奥の面が壁の手前の面にくっつくように手前へ出す
			stage::GateStyle gateStyle;
			gateStyle.scale = m_layout.gateScale;
			gateStyle.footZ = -RUIN_DEPTH * 0.5f - GATE_MODEL_DEPTH * m_layout.gateScale - GATE_WALL_GAP;
			m_gateRenderer.Init(gateStyle);

			stage::ClearTiming timing;
			timing.doorOpenTime = doorOpenTime;
			m_doorSequence.Init(timing);

			ClearGateLight();
			m_isInited = true;
		}


		void RuinEntrance::OpenDoor()
		{
			if (m_gate == nullptr)
			{
				return;
			}
			m_doorSequence.Start(*m_gate);
		}


		void RuinEntrance::Update(const float deltaTime)
		{
			if (!m_isInited)
			{
				return;
			}
			m_doorSequence.Update(deltaTime);
			m_groundRenderer.Update(m_ground);
			m_ruinRenderer.Update(m_ruin);
			m_gateRenderer.Update(m_ruin, m_doorSequence, deltaTime);
			ApplyGateLight();
		}


		void RuinEntrance::Draw(RenderContext& rc)
		{
			if (!m_isInited)
			{
				return;
			}
			m_groundRenderer.Draw(rc);
			m_ruinRenderer.Draw(rc);
			m_gateRenderer.Draw(rc);
		}


		float RuinEntrance::GetFloorHeight(const float z) const
		{
			// 門は大きくして手前へ出しているので、段の位置と高さも門の大きさと足元の奥行きに合わせる
			const float scale = m_layout.gateScale;
			const float footZ = m_gateRenderer.GetStyle().footZ;
			float height = 0.0f;
			for (const gate_step::Step& step : gate_step::STEPS)
			{
				const float frontZ = footZ + step.frontZ * scale;
				const float rate = math::SmoothStep(math::Clamp01((z - (frontZ - gate_step::STEP_UP_LENGTH)) / gate_step::STEP_UP_LENGTH));
				height = math::Lerp(height, step.topY * scale, rate);
			}
			return height;
		}


		void RuinEntrance::BuildGround()
		{
			using stage::StageBlock;
			m_ground.Clear();
			m_ground.SetName("TitleGround");
			// 画面の左右の外まで。下は画面の下の外まで
			m_ground.Add<StageBlock>("Ground", -1600.0f, -500.0f, 3200.0f, 500.0f);
		}


		void RuinEntrance::BuildRuin()
		{
			using stage::StageBlock;
			using stage::Goal;
			namespace shape = ruin_shape;

			m_ruin.Clear();
			m_ruin.SetName("TitleRuin");
			const float g = m_layout.gateX;

			// ---- 遺跡の建物（門の後ろも壁でふさぎ、右へ段々に高くなる） ----
			const float right = g + shape::RIGHT_END;
			const float tier2Bottom = shape::WALL_HEIGHT;
			const float tier3Bottom = tier2Bottom + shape::TIER2_HEIGHT;
			m_ruin.Add<StageBlock>("Wall", g + shape::WALL_LEFT, 0.0f, right - (g + shape::WALL_LEFT), shape::WALL_HEIGHT);
			m_ruin.Add<StageBlock>("Tier2", g + shape::TIER2_LEFT, tier2Bottom, right - (g + shape::TIER2_LEFT), shape::TIER2_HEIGHT);
			m_ruin.Add<StageBlock>("Tier3", g + shape::TIER3_LEFT, tier3Bottom, right - (g + shape::TIER3_LEFT), shape::TIER3_HEIGHT);

			// ---- 遺跡の外の崩れた柱と石（プレイヤーが走ってくる所の奥） ----
			m_ruin.Add<StageBlock>("PillarA", -470.0f, 0.0f, 50.0f, 230.0f);
			m_ruin.Add<StageBlock>("PillarACap", -485.0f, 230.0f, 80.0f, 25.0f);
			m_ruin.Add<StageBlock>("PillarB", -170.0f, 0.0f, 45.0f, 125.0f);
			m_ruin.Add<StageBlock>("FallenStoneA", -340.0f, 0.0f, 85.0f, 40.0f);
			m_ruin.Add<StageBlock>("FallenStoneB", 20.0f, 0.0f, 55.0f, 28.0f);

			// ---- 門（ゴールの門をそのまま使う。判定の下端の真ん中に門が立つ） ----
			m_gate = &m_ruin.Add<Goal>("Gate", g - GATE_GOAL_WIDTH * 0.5f, 0.0f, GATE_GOAL_WIDTH, GATE_GOAL_HEIGHT);
		}


		void RuinEntrance::ApplyGateLight()
		{
			const float openRate = (m_gate != nullptr) ? m_doorSequence.GetDoorOpenRate(*m_gate) : 0.0f;
			auto& light = SceneLight::GetInstance()->GetSceneLight();
			auto& point = light.ptLights[GATE_LIGHT_INDEX];
			// 門の足元（手前へ出している）からの位置
			const float footZ = m_gateRenderer.GetStyle().footZ;
			point.position = Vector3(m_layout.gateX, m_lightStyle.height, footZ + m_lightStyle.z);
			point.range = m_lightStyle.range;
			const float intensity = m_lightStyle.intensity * openRate;
			point.color = Vector3(m_lightStyle.color[0] * intensity, m_lightStyle.color[1] * intensity, m_lightStyle.color[2] * intensity);
			// 向きのない光（スポットライトにしない）
			point.angle = 0.0f;
			light.numPtLights = openRate > 0.0f ? GATE_LIGHT_INDEX + 1 : 0;
		}


		void RuinEntrance::ClearGateLight()
		{
			SceneLight::GetInstance()->GetSceneLight().numPtLights = 0;
		}
	}
}
