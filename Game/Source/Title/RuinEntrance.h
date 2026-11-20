#pragma once
#include "Source/Stage/Goal/ClearSequence.h"
#include "Source/Stage/Goal/GoalGateRenderer.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/StageRenderer.h"
#include "Source/Title/TitleParameter.h"


/**
 * @file RuinEntrance.h
 * @brief タイトル画面の遺跡の入口（地面・遺跡の建物・外の崩れた柱・ゴールと同じ門・扉の奥からもれる光）
 */
namespace app
{
	namespace title
	{
		/**
		 * @brief タイトル画面の遺跡の入口
		 * @details ・地形はステージと同じ地形ブロック（StageBlock）を C++ で並べる（JSON は使わない。宝物の間の TreasureRoom と同じ）
		 *            地面と遺跡の建物は奥行きを変えたいので、別の Stage と StageRenderer にしている
		 *              地面 … 奥行き 400（Z = -200〜200）。プレイヤーは地面の上の手前のほう（playerZ）を走る
		 *              建物 … 奥行き 100（Z = -50〜50）。プレイヤーより奥なので、プレイヤーが建物にめり込まない
		 *          ・門はゴールの門（GoalGateRenderer）をそのまま使う。建物のステージにゴールを1つ置き、
		 *            建物には門が入る口（門の幅より少し広い）を空けている
		 *          ・扉は ClearSequence で開ける（ゲームでゴールに触れたときと同じ開き方）。OpenDoor で開き始める
		 *          ・扉が開くほど、門の手前のポイントライト（シーンのポイントライト0番）が明るくなり、
		 *            門の前のプレイヤーと地面が光の色に照らされる（このクラスがなくなるときに消す）
		 */
		class RuinEntrance
		{
		public:
			RuinEntrance() = default;
			~RuinEntrance();
			// Model を持っているのでコピー禁止
			RuinEntrance(const RuinEntrance&) = delete;
			RuinEntrance& operator=(const RuinEntrance&) = delete;

			/**
			 * @param layout 置き方（門の位置）
			 * @param doorOpenTime 扉が開ききるまでの時間
			 */
			void Init(const RuinEntranceLayout& layout, float doorOpenTime);
			/** 扉を開け始める */
			void OpenDoor();
			/** 扉と炎を動かし、門の光を当てる（毎フレーム） */
			void Update(float deltaTime);
			/** 描く（地面 → 建物 → 門） */
			void Draw(RenderContext& rc);

		private:
			/** 地面を作る */
			void BuildGround();
			/** 遺跡の建物と、外の崩れた柱を作る。門の所にゴールを置く */
			void BuildRuin();
			/** 門の手前のポイントライトを、扉の開き具合に合わせて置く */
			void ApplyGateLight();
			/** 門の光を消す */
			static void ClearGateLight();

		private:
			/** 置き方 */
			RuinEntranceLayout m_layout;
			/** 扉の奥からもれる光 */
			GateLightStyle m_lightStyle;
			/** 地面 */
			stage::Stage m_ground;
			/** 遺跡の建物と外の柱（門のゴールもここ） */
			stage::Stage m_ruin;
			/** 地面の描画 */
			stage::StageRenderer m_groundRenderer;
			/** 建物の描画 */
			stage::StageRenderer m_ruinRenderer;
			/** 門 */
			stage::GoalGateRenderer m_gateRenderer;
			/** 扉の開き方（時間を数える） */
			stage::ClearSequence m_doorSequence;
			/** 門のゴール（m_ruin が持っている。扉の開き具合を聞くのに使う） */
			const stage::Goal* m_gate = nullptr;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
