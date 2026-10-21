/**
 * @file ColorBoxRenderer.h
 * @brief 色の付いた半透明の箱を、好きな数だけ描く（色センサーの窓・扉の色の印・色ガラスなど）
 * @details ・光と同じシェーダー（Assets/shader/lightBeam.fx）と箱のモデル（mirror_glass.tkm）を使う。
 *            色はシェーダーに渡す値で決めるので、色ごとにモデルやテクスチャを作らなくてよい。
 *          ・rgb を 1 より大きくすると、ブルームで光って見える（点灯した表現に使う）。
 *          ・毎フレーム Begin → Put（何個でも）→ Draw の順に呼ぶ。
 *            モデルは足りなくなったときだけ増やし、使い回す（LightBeamRenderer と同じ作り）。
 */
#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>


namespace app
{
	namespace render
	{
		/**
		 * @brief 色の付いた半透明の箱を描く
		 */
		class ColorBoxRenderer
		{
		public:
			/**
			 * @brief 箱1つぶんの見た目
			 */
			struct Look
			{
				/** 色（rgb。1 より大きいと光る）と不透明度（a） */
				float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				/** こちらを向いている面の不透明度の割合（1 で a のまま。小さいほど真ん中が透ける） */
				float frontOpacity = 1.0f;
				/** 縁の明るさ（1 でふつう。大きいほど縁が光る） */
				float edgeGlow = 1.0f;
			};


		public:
			/**
			 * @brief 初期化（モデルは必要になったときに読む）
			 * @param modelPath 1辺 modelSize の箱のモデル（原点は中心・Y-up）
			 */
			void Init(const char* modelPath = "Assets/modelData/gimmick/mirror_glass.tkm", float modelSize = 100.0f);
			/** @brief 今フレーム置いた箱を全部消す（毎フレーム、Put の前に呼ぶ） */
			void Begin();
			/**
			 * @brief 箱を1つ置く
			 * @param center 中心
			 * @param size 大きさ（X:幅 / Y:高さ / Z:奥行き）
			 * @param angleDeg Z 軸まわりの回転（度）
			 */
			void Put(const Vector3& center, const Vector3& size, const Look& look, float angleDeg = 0.0f);
			/** @brief 描く（半透明。ほかのモデルを全部 Draw したあとに呼ぶ） */
			void Draw(RenderContext& rc);
			/** @brief 今フレーム置いた数 */
			size_t GetCount() const { return m_count; }


		public:
			ColorBoxRenderer();
			// Model を持っているのでコピー禁止
			ColorBoxRenderer(const ColorBoxRenderer&) = delete;
			ColorBoxRenderer& operator=(const ColorBoxRenderer&) = delete;


		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す値（lightBeam.fx の DirectionLightCb と同じ並び。LightBeamRenderer::Constants と同じ）
			 */
			struct Constants
			{
				SceneLightData light;
				float color[4];			// rgb = 色, a = 不透明度
				float param[4];			// x = こちらを向いている面の不透明度の割合, y = 縁の明るさ
				float startCap[4];		// 切り口（使わないので 0）
				float endCap[4];		// 切り口（使わないので 0）
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");

			/**
			 * @brief 箱1つぶんのモデルと、そのモデル専用のシェーダーに渡す値
			 */
			struct BoxModel
			{
				Model model;
				Constants constants;
			};


		private:
			/** @brief 箱のモデルを1つ取り出す（足りなければ作る） */
			BoxModel& Acquire();


		private:
			/** 箱のモデルのパス */
			std::string m_modelPath;
			/** 箱のモデルの1辺の長さ */
			float m_modelSize;
			/** 箱のモデル（unique_ptr なので、増えても constants の場所は変わらない。Model はこの場所を覚えている） */
			std::vector<std::unique_ptr<BoxModel>> m_boxes;
			/** 今フレーム使っている数 */
			size_t m_count;
			/** 初期化済みか */
			bool m_isInited;
		};
	}
}
