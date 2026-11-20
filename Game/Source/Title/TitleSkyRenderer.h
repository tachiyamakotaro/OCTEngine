#pragma once
#include "Source/Title/TitleParameter.h"
#include <type_traits>


/**
 * @file TitleSkyRenderer.h
 * @brief タイトル画面の夕暮れの空（いちばん奥の大きな板に、titleSky.fx で描く）
 */
namespace app
{
	namespace title
	{
		/**
		 * @brief タイトル画面の空
		 * @details ・Box.tkm を薄く大きく伸ばした板を、カメラの注視点の奥（RuinEntranceLayout::skyZ）に置く
		 *          ・色は画面の位置だけで決める（シェーダー Assets/shader/titleSky.fx）
		 *              上の紺 → 紫 → 地平線の夕焼け、沈みかけの太陽、遠くの山と近くの丘のシルエット、上のほうの星
		 *          ・見た目の値は SkyStyle
		 */
		class TitleSkyRenderer
		{
		public:
			TitleSkyRenderer() = default;
			~TitleSkyRenderer() = default;
			// Model は定数バッファの場所を覚えているのでコピー禁止
			TitleSkyRenderer(const TitleSkyRenderer&) = delete;
			TitleSkyRenderer& operator=(const TitleSkyRenderer&) = delete;

			/**
			 * @param layout 板を置く奥行きと大きさ・カメラの注視点
			 * @param style 見た目
			 */
			void Init(const RuinEntranceLayout& layout, const SkyStyle& style = SkyStyle());
			/** 星のまたたきの時間を進めて、シェーダーに渡す値を入れる（毎フレーム） */
			void Update(float deltaTime);
			/** 描く（不透明。ほかのモデルより先に呼ぶ） */
			void Draw(RenderContext& rc);

		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す空の値（titleSky.fx の MODEL_CB_EXTRA と同じ並び）
			 */
			struct ShaderValues
			{
				/** rgb = いちばん上の色 */
				Vector4 topColor;
				/** rgb = 真ん中の色, a = その高さ（v） */
				Vector4 middleColor;
				/** rgb = 地平線の色, a = 地平線の高さ（v） */
				Vector4 horizonColor;
				/** x = 1 / 画面の幅, y = 1 / 画面の高さ, z = 時間 */
				Vector4 screen;
				/** xy = 太陽の位置, z = 太陽の大きさ, w = まわりの光の広がり */
				Vector4 sun;
				/** rgb = 太陽の色, a = 光る強さ */
				Vector4 sunColor;
				/** rgb = 遠くの山の色, a = ふもとの高さ */
				Vector4 farMountain;
				/** rgb = 近くの丘の色, a = ふもとの高さ */
				Vector4 nearMountain;
				/** x = 遠くの山の高さ, y = 近くの丘の高さ, z = もやの濃さ, w = 星の明るさ */
				Vector4 mountainShape;
			};
			/** シェーダーに渡す値（前半は model.fx と同じライトの値、後ろに空の値をつなげる） */
			struct Constants
			{
				SceneLightData light;
				ShaderValues values;
			};
			static_assert(sizeof(ShaderValues) == 16 * 9, "ShaderValues は float4 が9つ（titleSky.fx と同じ並び）");

			/** 見た目の値をシェーダーに渡す形にする（時間は Update で入れる） */
			void WriteStyle();

		private:
			/** 見た目 */
			SkyStyle m_style;
			/** 空の板 */
			Model m_model;
			/** シェーダーに渡す値（m_model が場所を覚えているので動かさない） */
			Constants m_constants;
			/** 星のまたたきの時間 */
			float m_time = 0.0f;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
