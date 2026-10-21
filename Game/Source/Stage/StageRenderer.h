/**
 * @file StageRenderer.h
 * @brief ステージの地形をモデルで描く
 * @details 今あるアセットは「黄色い床」（ground.tkm：50000×50000 の平らな板）だけなので、
 *          それを各ブロックの上面に、ブロックの幅 × 奥行きに縮めて置いている。
 *          奥の鏡があるときは、鏡に映った床も描く（鏡の面で反転したコピーを置く）。
 *
 *          立方体のモデルができたら、Config の modelPath / modelSize / isPlane を変えれば
 *          ブロック全体（前面・側面込み）を描けるようになる。
 *          動く床・動く壁（MovingBlock）は、どれが動くのかわかるように別のモデル（青い箱）で描く。
 *          動く床・動く壁に付いたソーラーパネルは、セルが並んだ模様をシェーダーで描き、たまった電気の量でセルが黒 → 灰色 → 白に変わって光る
 *          （シェーダーは Assets/shader/solarPanel.fx）。
 *          扉・光・反射鏡などはまだ StageDebugOverlay で描いている。
 */
#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		class Stage;
		struct BackMirror;


		/**
		 * @brief ステージの地形を描く
		 */
		class StageRenderer
		{
		public:
			/**
			 * @brief 使うモデルの設定
			 */
			struct Config
			{
				/** モデルのパス */
				std::string modelPath = "Assets/modelData/ground.tkm";
				/** 動く床・動く壁のモデルのパス（modelPath と同じ形・大きさ・原点で、色だけ違うモデルにする） */
				std::string movingBlockModelPath = "Assets/modelData/moving_block.tkm";
				/** ソーラーパネルのモデルのパス（modelPath と同じ形・大きさ・原点。テクスチャは真っ白で、模様はシェーダーで描く） */
				std::string solarPanelModelPath = "Assets/modelData/solar_panel.tkm";
				/** ソーラーパネルのセルの色（空のとき。黒） */
				float panelEmptyColor[3] = { 0.02f, 0.02f, 0.025f };
				/** ソーラーパネルのセルの色（満タンのとき。白。黒 → 灰色 → 白と変わっていく） */
				float panelFullColor[3] = { 0.95f, 0.97f, 1.0f };
				/** 満タンのとき、パネルが自分で光る強さ（0 なら光らない） */
				float panelGlow = 0.6f;
				/** モデルの上方向（ground.tkm は Z-up） */
				EnModelUpAxis upAxis = enModelUpAxisZ;
				/** モデルの元の大きさ（X:幅 / Y:高さ / Z:奥行き。板なら高さは使わない） */
				Vector3 modelSize = Vector3(50000.0f, 1.0f, 50000.0f);
				/** 平らな板か（true なら上面だけ置く。false なら箱として拡大する） */
				bool isPlane = true;
				/** 箱の原点が底面の中心か（false なら箱の中心） */
				bool isPivotAtBottom = false;
				/** ステージの奥行き（Z方向の厚み） */
				float depth = 300.0f;
				/** 影を受けるか */
				bool isShadowReceiver = true;
			};


		public:
			/**
			 * @brief 初期化（モデルの設定を決める）
			 */
			void Init(const Config& config);
			/**
			 * @brief ステージに合わせてモデルの数・位置・大きさを更新する（毎フレーム）
			 * @details エディタでブロックを足したり動かしたりしても、そのまま追従する
			 */
			void Update(const Stage& stage);
			/**
			 * @brief 描く
			 */
			void Draw(RenderContext& rc);
			/**
			 * @brief 設定を取得（ImGui での調整用）
			 */
			inline Config& GetConfig() { return m_config; }


		public:
			StageRenderer() = default;
			// ModelRender を持っているのでコピー禁止
			StageRenderer(const StageRenderer&) = delete;
			StageRenderer& operator=(const StageRenderer&) = delete;


		private:
			/**
			 * @brief 同じモデルで描く物のまとまり（地形ブロック / 動く床・動く壁）
			 */
			struct ModelGroup
			{
				/** 今フレーム描く範囲（毎回メモリ確保しないよう持っておく） */
				std::vector<math::Rect> rects;
				/** モデル（範囲の数だけ使う） */
				std::vector<std::unique_ptr<ModelRender>> models;
				/** 鏡に映ったモデル（範囲の数だけ使う） */
				std::vector<std::unique_ptr<ModelRender>> reflectionModels;
				/** 今フレーム描くモデルの数 */
				size_t activeCount = 0;
				/** 今フレーム描く鏡に映ったモデルの数 */
				size_t activeReflectionCount = 0;
			};


			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;
			/**
			 * @brief ソーラーパネルのシェーダーに渡す値（solarPanel.fx の DirectionLightCb と同じ並び）
			 * @details 前半は model.fx と同じライトの値。その後ろにパネルの値を float4 でつなげる。
			 */
			struct PanelConstants
			{
				SceneLightData light;
				float full[4];		// rgb = 満タンのときのセルの色, a = たまった量（0〜1）
				float empty[4];		// rgb = 空のときのセルの色, a = 満タンのとき自分で光る強さ
				float origin[4];	// xyz = パネルの角（模様はここからの距離で作るので、足場と一緒に模様も動く）
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");
			/**
			 * @brief ソーラーパネル1枚ぶんのモデルと、そのモデル専用のシェーダーに渡す値
			 * @details パネルごとに色が違うので、1枚ずつ自分の定数バッファを持つ（LightBeamRenderer の光と同じ作り）。
			 *          ModelRender は model.fx 固定なので、Model を直接持っている。
			 */
			struct PanelModel
			{
				Model model;
				PanelConstants constants;
			};


		private:
			/**
			 * @brief まとまり1つぶんのモデルを、rects に合わせて置く
			 * @param modelPath 使うモデル
			 */
			void UpdateGroup(ModelGroup& group, const std::string& modelPath, const BackMirror& mirror);
			/** @brief まとまり1つぶんを描く */
			void DrawGroup(RenderContext& rc, ModelGroup& group);
			/**
			 * @brief 矩形1つぶんのモデルを置く
			 * @param isReflected 鏡に映った像として置くか
			 * @param planeZ 鏡の面のZ座標
			 */
			void Place(ModelRender& model, const math::Rect& rect, bool isReflected, float planeZ);
			/**
			 * @brief 必要な数だけモデルを用意する
			 * @param models 用意する先
			 * @param cullMode カリング（鏡に映った像は表裏が逆になるので FRONT）
			 * @param modelPath 使うモデル
			 */
			void Reserve(std::vector<std::unique_ptr<ModelRender>>& models, size_t count, D3D12_CULL_MODE cullMode, const std::string& modelPath);
			/** @brief 矩形の大きさに合わせた、モデルの位置と拡大率（Place と同じ計算） */
			void CalcTransform(const math::Rect& rect, float depth, Vector3& outPosition, Vector3& outScale) const;
			/**
			 * @brief ソーラーパネルを1枚置く
			 * @param panelRect パネルの範囲
			 * @param brightness 光り具合（0=黒 〜 1=白く光る。MovingBlock::GetPanelBrightness）
			 * @param isFront 手前の面に貼る薄い板にするか（false なら、奥行きいっぱいの帯）
			 */
			void PlacePanel(const math::Rect& panelRect, float brightness, bool isFront);
			/** @brief ソーラーパネルのモデルを1つ取り出す（足りなければ作る） */
			PanelModel& AcquirePanel();


		private:
			/** 設定 */
			Config m_config;
			/** 地形ブロック */
			ModelGroup m_blocks;
			/** 動く床・動く壁 */
			ModelGroup m_movingBlocks;
			/** ソーラーパネル（unique_ptr なので、増えても constants の場所は変わらない。Model はこの場所を覚えている） */
			std::vector<std::unique_ptr<PanelModel>> m_panels;
			/** 今フレーム使っているソーラーパネルの数 */
			size_t m_panelCount = 0;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
