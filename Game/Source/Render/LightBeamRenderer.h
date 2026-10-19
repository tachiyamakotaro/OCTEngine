/**
 * @file LightBeamRenderer.h
 * @brief 光源から出る光を、半透明に少し光らせて描く
 * @details ・光の計算の結果（Stage::GetLightResult の線分）を、1本ずつ箱のモデルで描く。
 *            真ん中は透けるので、光の中にいるプレイヤーも見える（シェーダーは Assets/shader/lightBeam.fx）。
 *          ・太さは光源の isWideBeam で決まる（細い光は Config の thinBeamWidth）。
 *          ・鏡で反射する所は、光の端を鏡の面に沿って切る（光が鏡からはみ出さないように）。
 *            光ごとに切り口の位置が違うので、光は1本ずつ自分専用の定数バッファ（BeamModel::constants）を持つ。
 *          ・lightBeam.fx は model.fx を include しているので、ライトの値（カメラの位置など）は model.fx と同じ並びで渡す。
 *
 *          エンジンの ModelRender は model.fx 固定なので、MirrorGlass と同じく Model を直接持っている。
 */
#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>


namespace app
{
	namespace stage
	{
		class Stage;
	}


	namespace render
	{
		/**
		 * @brief 光を半透明で描く
		 */
		class LightBeamRenderer
		{
		public:
			/**
			 * @brief 設定
			 */
			struct Config
			{
				/** 細い光（光源の isWideBeam が false）の太さ */
				float thinBeamWidth = 8.0f;
				/** 光を描く箱のモデル（1辺100の箱・原点は中心・Y-up） */
				std::string modelPath = "Assets/modelData/gimmick/mirror_glass.tkm";
				/** 箱のモデルの1辺の長さ */
				float modelSize = 100.0f;
				/** 光の色（rgb）と不透明度（a） */
				float color[4] = { 1.0f, 0.95f, 0.6f, 0.45f };
				/** こちらを向いている面の不透明度の割合（小さいほど真ん中が透ける。光の上下の縁は a のまま） */
				float frontOpacity = 0.5f;
				/** 光の上下の縁の明るさ（1 より大きいとブルームで少し光る） */
				float edgeGlow = 1.8f;
			};


		public:
			/** @brief 初期化（モデルは必要になったときに読む） */
			void Init(const Config& config);
			/** @brief 光の線分に合わせてモデルを置く（毎フレーム、ステージの更新のあとに呼ぶ） */
			void Update(const stage::Stage& stage);
			/** @brief 描く（半透明。ほかのモデルを全部 Draw したあとに呼ぶ） */
			void Draw(RenderContext& rc);
			/** @brief 設定（ImGui での調整用） */
			inline Config& GetConfig() { return m_config; }


		public:
			LightBeamRenderer();
			// Model を持っているのでコピー禁止
			LightBeamRenderer(const LightBeamRenderer&) = delete;
			LightBeamRenderer& operator=(const LightBeamRenderer&) = delete;


		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す値（lightBeam.fx の DirectionLightCb と同じ並び）
			 * @details 前半は model.fx と同じライトの値。その後ろに光の値を float4 でつなげる。
			 */
			struct Constants
			{
				SceneLightData light;
				float color[4];			// rgb = 色, a = 不透明度
				float param[4];			// x = こちらを向いている面の不透明度の割合, y = 縁の明るさ
				float startCap[4];		// 光の始まりの切り口：xy = 面の向き（こちら側を残す）, zw = 面の上の点。xy が 0 なら切らない
				float endCap[4];		// 光の終わりの切り口（同じ形）
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");

			/**
			 * @brief 光1本ぶんのモデルと、そのモデル専用のシェーダーに渡す値
			 */
			struct BeamModel
			{
				Model model;
				Constants constants;
			};


		private:
			/** @brief 光のモデルを1つ取り出す（足りなければ作る） */
			BeamModel& AcquireBeam();


		private:
			/** 設定 */
			Config m_config;
			/** 光のモデル（unique_ptr なので、増えても constants の場所は変わらない。Model はこの場所を覚えている） */
			std::vector<std::unique_ptr<BeamModel>> m_beams;
			/** 今フレーム使っている数 */
			size_t m_beamCount;
			/** 初期化済みか */
			bool m_isInited;
		};
	}
}
