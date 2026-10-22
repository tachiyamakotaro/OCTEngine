/**
 * @file ColorLightRenderer.h
 * @brief 色の光のギミック（色センサー・扉の色の印・色の付いた光源）を描く
 * @details GimmickRenderer とは別のクラスにしている（ほかのギミックのブランチとぶつかりにくくするため）。
 *          ・色センサー（ColorReceiver）… 本体（受光器の消灯モデル）＋ 手前にセンサーの色の窓
 *              点灯（決められた色の光が当たっている）… 窓が明るく光る（ブルーム）
 *              違う色の光が当たっている           … 窓が点滅する（「光は届いているが色が違う」と伝える）
 *              光が当たっていない                 … 窓は暗い
 *          ・扉 … 色センサーにつながった扉の手前に、そのセンサーの色の印（どの色で開くか、扉を見ればわかる）
 *          ・光源 … 白以外の光を出す光源の手前に、光の色の窓
 *          色の窓と印は ColorBoxRenderer（光と同じ半透明のシェーダー）で描くので、色ごとのモデルはいらない。
 */
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Source/Core/Math/Rect.h"
#include "Source/Render/ColorBoxRenderer.h"
#include "Source/Stage/LightColor.h"


namespace app
{
	namespace stage
	{
		class Stage;


		/**
		 * @brief 色の光のギミックを描く
		 */
		class ColorLightRenderer
		{
		public:
			/**
			 * @brief 設定
			 */
			struct Config
			{
				/** 色センサーの本体のモデル（1辺100の箱・原点は中心） */
				std::string receiverModelPath = "Assets/modelData/gimmick/receiver_off.tkm";
				/** 色の窓・印に使う箱のモデル（1辺100の箱・原点は中心） */
				std::string boxModelPath = "Assets/modelData/gimmick/mirror_glass.tkm";
				/** モデルの上方向（仮モデルは Y-up） */
				EnModelUpAxis upAxis = enModelUpAxisY;
				/** モデルの元の大きさ */
				float modelSize = 100.0f;

				// ---- 色センサー ----
				/** 本体の奥行き（受光器と同じ） */
				float receiverDepth = 60.0f;
				/** 色の窓の大きさ（本体の幅・高さに対する割合） */
				float lensRate = 0.65f;
				/** 色の窓の厚さ */
				float lensThickness = 4.0f;

				// ---- 明るさ（色に掛ける。1 より大きいとブルームで光る）----
				/** 光が当たっていないとき */
				float offBrightness = 0.45f;
				/** 決められた色の光が当たっているとき */
				float onBrightness = 3.0f;
				/** 違う色の光が当たっているときの点滅の明るい側 */
				float wrongBrightness = 1.3f;
				/** 違う色の光が当たっているときの点滅の速さ（1秒あたりの回数） */
				float wrongBlinkHz = 4.0f;

				// ---- 扉の色の印 ----
				/** 扉の奥行き（GimmickRenderer の doorDepth と同じにする） */
				float doorDepth = 120.0f;
				/** 印の一辺（扉の幅より大きければ扉の幅の 0.8 倍） */
				float doorMarkSize = 30.0f;
				/** 印が2つ以上あるときの間隔（印の一辺に対する割合） */
				float doorMarkSpacingRate = 1.5f;

				// ---- 光源の色の窓 ----
				/** 光源の奥行き（GimmickRenderer の lightSourceDepth と同じにする） */
				float lightSourceDepth = 40.0f;
				/** 色の窓の一辺 */
				float lightSourceLensSize = 22.0f;
				/** 光を出している光源の窓の明るさ */
				float lightSourceBrightness = 2.0f;
			};


		public:
			/** @brief 初期化（モデルは必要になったときに読み込む） */
			void Init(const Config& config);
			/** @brief ステージに合わせてモデルを置く（毎フレーム、ステージの更新のあとに呼ぶ） */
			void Update(const Stage& stage, float deltaTime);
			/** @brief 色センサーの本体を描く（不透明。GimmickRenderer の Draw と同じ所で呼ぶ） */
			void DrawOpaque(RenderContext& rc);
			/** @brief 色の窓・印を描く（半透明。ほかのモデルを全部 Draw したあと、光より前に呼ぶ） */
			void DrawTranslucent(RenderContext& rc);
			/** @brief 設定（ImGui での調整用） */
			inline Config& GetConfig() { return m_config; }


		public:
			ColorLightRenderer() = default;
			// モデルを持っているのでコピー禁止
			ColorLightRenderer(const ColorLightRenderer&) = delete;
			ColorLightRenderer& operator=(const ColorLightRenderer&) = delete;


		private:
			/** @brief 色の箱を1つ置く（color の色に brightness を掛ける） */
			void PutColorBox(const math::Vec2& center, float z, float width, float height, float thickness, LightColor color, float brightness);
			/** @brief 色センサーの本体のモデルを1つ取り出す（足りなければ作る） */
			ModelRender& AcquireReceiverModel();


		private:
			/** 設定 */
			Config m_config;
			/** 色センサーの本体のモデル（使い回す） */
			std::vector<std::unique_ptr<ModelRender>> m_receiverModels;
			/** 今フレーム使っている本体のモデルの数 */
			size_t m_receiverCount = 0;
			/** 色の窓・印 */
			render::ColorBoxRenderer m_colorBoxes;
			/** 点滅に使う時間 */
			float m_time = 0.0f;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
