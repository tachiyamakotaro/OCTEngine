/**
 * @file RuinEntrance.cpp
 * @brief タイトル画面の遺跡の入口
 */
#include "stdafx.h"
#include "RuinEntrance.h"


namespace app
{
	namespace title
	{
		namespace
		{
			/** 門の光に使うシーンのポイントライトの番号（宝物の間の宝石の光と同じ 0 番。同時には使わない） */
			constexpr int GATE_LIGHT_INDEX = 0;

			/** 地面の奥行き（Z = -200〜200）と、建物の奥行き（Z = -50〜50） */
			constexpr float GROUND_DEPTH = 400.0f;
			constexpr float RUIN_DEPTH = 100.0f;

			/**
			 * @brief 遺跡の建物の形（門の真ん中 gateX からの距離。地面の上が y = 0）
			 * @details 門のモデル（goal_gate.tkm）は幅 252・高さ 272。口は門より少し広く・高くして、門が建物にめり込まないようにする
			 *
			 *                      ┌──────────── 3段目
			 *                ┌─────┘
			 *            ┌───┘ 2段目
			 *          ┌─┴─────────────────── まぐさ石（口の上）
			 *          │左│  門の口  │右の壁 → 画面の右の外まで
			 *     ─────┴──┴──────────┴──────────── 地面
			 */
			namespace ruin_shape
			{
				/** 門の口の半分の幅と、口の高さ */
				constexpr float OPENING_HALF_WIDTH = 130.0f;
				constexpr float OPENING_HEIGHT = 285.0f;
				/** 口の左の柱の幅 */
				constexpr float LEFT_JAMB_WIDTH = 70.0f;
				/** まぐさ石の高さと、左の柱からはみ出す長さ */
				constexpr float LINTEL_HEIGHT = 80.0f;
				constexpr float LINTEL_OVERHANG = 20.0f;
				/** 2段目・3段目の左端（門の真ん中から）と高さ */
				constexpr float TIER2_LEFT = -150.0f;
				constexpr float TIER2_HEIGHT = 90.0f;
				constexpr float TIER3_LEFT = -60.0f;
				constexpr float TIER3_HEIGHT = 400.0f;
				/** 建物の右端（門の真ん中から。画面の右の外まで伸ばす） */
				constexpr float RIGHT_END = 900.0f;
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
			m_gateRenderer.Init();

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

			// ---- 遺跡の建物（門の口を空けて、右へ段々に高くなる） ----
			const float openingLeft = g - shape::OPENING_HALF_WIDTH;
			const float openingRight = g + shape::OPENING_HALF_WIDTH;
			const float jambLeft = openingLeft - shape::LEFT_JAMB_WIDTH;
			const float right = g + shape::RIGHT_END;
			const float lintelTop = shape::OPENING_HEIGHT + shape::LINTEL_HEIGHT;
			const float tier2Top = lintelTop + shape::TIER2_HEIGHT;

			m_ruin.Add<StageBlock>("JambLeft", jambLeft, 0.0f, shape::LEFT_JAMB_WIDTH, shape::OPENING_HEIGHT);
			m_ruin.Add<StageBlock>("WallRight", openingRight, 0.0f, right - openingRight, shape::OPENING_HEIGHT);
			m_ruin.Add<StageBlock>("Lintel", jambLeft - shape::LINTEL_OVERHANG, shape::OPENING_HEIGHT,
				right - (jambLeft - shape::LINTEL_OVERHANG), shape::LINTEL_HEIGHT);
			m_ruin.Add<StageBlock>("Tier2", g + shape::TIER2_LEFT, lintelTop, right - (g + shape::TIER2_LEFT), shape::TIER2_HEIGHT);
			m_ruin.Add<StageBlock>("Tier3", g + shape::TIER3_LEFT, tier2Top, right - (g + shape::TIER3_LEFT), shape::TIER3_HEIGHT);

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
			point.position = Vector3(m_layout.gateX, m_lightStyle.height, m_lightStyle.z);
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
