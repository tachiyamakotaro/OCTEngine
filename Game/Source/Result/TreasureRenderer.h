#pragma once
#include "Source/Result/ResultParameter.h"
#include <type_traits>


/**
 * @file TreasureRenderer.h
 * @brief 宝物の間の宝箱・金貨・光の宝石・まわりの光を描く
 */
namespace app
{
	namespace result
	{
		class CelebrationTimeline;


		/**
		 * @brief 宝物の間の宝箱・金貨・光の宝石・まわりの光を描く
		 * @details ・モデルは Assets/modelData/treasure/（Tools/gen_treasure_models.py で作った）、シェーダーは Assets/shader/treasure.fx
		 *          ・ふたの開き具合・宝石の浮かび具合は CelebrationTimeline から受け取る（このクラスは時間を数えない）
		 *          ・宝石の光は、シーンのポイントライトを1つ使って、床・宝箱・プレイヤーを照らす（ApplyGemLight）。
		 *            シーンのライトはどのシーンでも同じ物なので、シーンを出るときに ClearGemLight で消す
		 *          ・光の筋と丸い光は半透明なので、ほかのモデルより後に登録する（DrawGlow）
		 *          ・ModelRender は model.fx 固定なので Model を直接持っている
		 */
		class TreasureRenderer
		{
		public:
			TreasureRenderer() = default;
			~TreasureRenderer() = default;
			// Model を持っている（Model は定数バッファの場所を覚えている）のでコピー禁止
			TreasureRenderer(const TreasureRenderer&) = delete;
			TreasureRenderer& operator=(const TreasureRenderer&) = delete;

			/**
			 * @param layout 宝箱を置く場所
			 * @param style 見た目
			 */
			void Init(const TreasureRoomLayout& layout, const TreasureStyle& style = TreasureStyle());
			/** 演出の進み具合に合わせて動かす（毎フレーム） */
			void Update(const CelebrationTimeline& timeline);
			/** 宝石の光で、まわりを照らす（シーンのポイントライトの 0 番を使う。毎フレーム、Update のあとに呼ぶ） */
			void ApplyGemLight() const;
			/** 宝石の光を消す（シーンを出るときに呼ぶ。ほかのシーンにポイントライトを残さない） */
			static void ClearGemLight();
			/** 不透明な物（宝箱・金貨・宝石）を描く */
			void Draw(RenderContext& rc);
			/** 半透明な物（光の筋・丸い光）を描く（ほかのモデルを全部登録したあとに呼ぶ） */
			void DrawGlow(RenderContext& rc);
			/** 宝石の位置（丸が開く中心などに使う） */
			const Vector3& GetGemPosition() const { return m_gemPosition; }

		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/** シェーダーに渡す値（treasure.fx の MODEL_CB_EXTRA と同じ並び） */
			struct ShaderValues
			{
				/** rgb = 宝石の色, a = 光る強さ */
				Vector4 gemColor;
				/** rgb = 光の筋の色, a = 光る強さ */
				Vector4 rayColor;
				/** x = 時間, y = 光の出具合（0〜1） */
				Vector4 param;
			};
			/** シェーダーに渡す値（前半は model.fx と同じライトの値） */
			struct Constants
			{
				SceneLightData light;
				ShaderValues values;
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");

			/** モデルを読み込む（全部のモデルが同じ Constants を使う） */
			void InitModel(Model& model, const char* tkmFilePath, const char* psEntryPointFunc, bool isTranslucent);

		private:
			/** 置く場所 */
			TreasureRoomLayout m_layout;
			/** 見た目 */
			TreasureStyle m_style;
			/** シェーダーに渡す値（全部のモデルで同じ物を使う） */
			Constants m_constants;

			Model m_chest;
			Model m_lid;
			Model m_gem;
			Model m_rays;
			Model m_halo;
			Model m_coinsLeft;
			Model m_coinsRight;

			/** 宝石の位置 */
			Vector3 m_gemPosition;
			/** 光の出具合（0〜1） */
			float m_glowRate = 0.0f;
			/** 宝石を描くか（浮かび始めるまでは、閉じた箱の中なので描かない） */
			bool m_isGemVisible = false;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
