#pragma once
#include "Source/Stage/Goal/GoalParameter.h"
#include <memory>
#include <type_traits>
#include <vector>


/**
 * @file GoalGateRenderer.h
 * @brief ゴールの門（遺跡の扉）を描く（門・2枚の扉・扉の奥の光・松明の炎）
 */
namespace app
{
	namespace stage
	{
		class Stage;
		class Goal;
		class ClearSequence;


		/**
		 * @brief ゴールの門を描く
		 * @details ・ゴールの矩形の下端の真ん中に、門を置く（矩形の大きさを変えても門の大きさは変わらない）
		 *            大きさは GateStyle::scale、奥行きは GateStyle::footZ（ゲームではどちらもそのまま＝1 と 0）
		 *          ・モデルは Assets/modelData/object/ の goal_～.tkm（Tools/gen_goal_models.py で作った）
		 *              goal_gate   … 門（段・柱・枠・まぐさ石・松明の金具）。ModelRender（model.fx）で描く
		 *              goal_door_l / goal_door_r … 扉。ちょうつがいを原点にして、Y 軸まわりに回して奥へ開く
		 *              goal_light  … 扉の奥の光。goal_flame … 松明の炎。どちらも goalGlow.fx で光らせる
		 *          ・扉の開き具合は ClearSequence に答えてもらう（触れたゴールの扉だけ開く）
		 *          ・調整用の ImGui は GoalEditor にある（このクラスは ImGui を使わない）
		 */
		class GoalGateRenderer
		{
		public:
			GoalGateRenderer() = default;
			~GoalGateRenderer() = default;
			// Model を持っている（Model は定数バッファの場所を覚えている）のでコピー禁止
			GoalGateRenderer(const GoalGateRenderer&) = delete;
			GoalGateRenderer& operator=(const GoalGateRenderer&) = delete;

			/**
			 * @brief 初期化（モデルは必要になったときに読み込む）
			 * @param style 見た目（ResetStyle で戻る値にもなる）
			 */
			void Init(const GateStyle& style = GateStyle());
			/**
			 * @brief ゴールごとに門を置く（毎フレーム。ゴールの判定のあとに呼ぶ）
			 * @param sequence 扉の開き具合を答えてもらう
			 * @param deltaTime 炎のゆらめきの時間を進める
			 */
			void Update(const stage::Stage& stage, const ClearSequence& sequence, float deltaTime);
			/** 描く（不透明。GimmickRenderer の Draw のあとに呼ぶ） */
			void Draw(RenderContext& rc);

			// ---- 調整値 ----
			const GateStyle& GetStyle() const { return m_style; }
			GateStyle& GetStyleForEdit() { return m_style; }
			/** 見た目を Init で渡された値に戻す */
			void ResetStyle() { m_style = m_initialStyle; }

			/** 今フレーム置いた門の数（確認用） */
			size_t GetCount() const { return m_count; }

		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す光の値（goalGlow.fx の MODEL_CB_EXTRA と同じ並び）
			 */
			struct GlowValues
			{
				/** rgb = まわりの色, a = 光る強さ */
				Vector4 color;
				/** rgb = 真ん中の色, a = 光る強さ */
				Vector4 coreColor;
			};
			/**
			 * @brief シェーダーに渡す値（前半は model.fx と同じライトの値、後ろに光の値をつなげる）
			 */
			struct GlowConstants
			{
				SceneLightData light;
				GlowValues glow;
			};
			static_assert(sizeof(GlowValues) == 16 * 2, "GlowValues は float4 が2つ（goalGlow.fx と同じ並び）");
			/**
			 * @brief 光る物1つぶん（モデルと、そのモデル専用のシェーダーに渡す値）
			 */
			struct GlowModel
			{
				Model model;
				GlowConstants constants;

				/** 光る色を決める */
				void SetColor(const GlowColor& color, const GlowColor& core);
				/** 置く */
				void Place(const Vector3& position, const Vector3& scale);
			};
			/**
			 * @brief 門1つぶん
			 */
			struct Gate
			{
				ModelRender frame;
				ModelRender doorLeft;
				ModelRender doorRight;
				GlowModel light;
				GlowModel flameLeft;
				GlowModel flameRight;
			};

		private:
			/** 門のモデルを1つ取り出す（足りなければ作る） */
			Gate& Acquire();
			/** 光る物のモデルを読み込む */
			void InitGlowModel(GlowModel& glow, const char* tkmFilePath, const char* psEntry);
			/**
			 * @brief 門と扉を置く
			 * @param foot 門の足元（ゴールの矩形の下端の真ん中）
			 * @param openRate 扉の開き具合（0〜1）
			 */
			void PlaceGate(Gate& gate, const Vector3& foot, float openRate);
			/** 扉の奥の光を置く（開くほど明るくなる） */
			void PlaceLight(Gate& gate, const Vector3& foot, float openRate);
			/**
			 * @brief 松明の炎を置く
			 * @param seed 門ごとにゆらめきをずらす数
			 */
			void PlaceFlames(Gate& gate, const Vector3& foot, float seed);
			/**
			 * @brief 炎1つの大きさ（ゆらめきを入れる）
			 * @param phase ゆらめきをずらす角度
			 */
			Vector3 CalcFlameScale(float phase) const;
			/** 門の足元（ゴールの矩形の下端の真ん中。奥行きは GateStyle::footZ） */
			Vector3 GetFoot(const stage::Goal& goal) const;
			/** 門の大きさ（GateStyle::scale を3つの向きにそろえた物） */
			Vector3 GetScale() const;

		private:
			/** 見た目 */
			GateStyle m_style;
			/** Init で渡された見た目 */
			GateStyle m_initialStyle;
			/** 門（使い回す） */
			std::vector<std::unique_ptr<Gate>> m_gates;
			/** 今フレーム置いた門の数 */
			size_t m_count = 0;
			/** 炎のゆらめきの時間 */
			float m_time = 0.0f;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
