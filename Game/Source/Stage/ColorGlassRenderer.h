/**
 * @file ColorGlassRenderer.h
 * @brief 色ガラスを半透明の色の板で描く
 * @details 色は ColorBoxRenderer（光と同じ半透明のシェーダー）で付けるので、色ごとのモデルはいらない。
 *          光が当たっている間は少し明るくする。
 */
#pragma once
#include "Source/Render/ColorBoxRenderer.h"


namespace app
{
	namespace stage
	{
		class Stage;


		/**
		 * @brief 色ガラスを描く
		 */
		class ColorGlassRenderer
		{
		public:
			/**
			 * @brief 設定
			 */
			struct Config
			{
				/** 箱のモデル（1辺100の箱・原点は中心） */
				const char* boxModelPath = "Assets/modelData/gimmick/mirror_glass.tkm";
				/** モデルの元の大きさ */
				float modelSize = 100.0f;
				/** ガラスの奥行き */
				float depth = 120.0f;
				/** 不透明度 */
				float opacity = 0.35f;
				/** こちらを向いている面の不透明度の割合（小さいほど真ん中が透ける） */
				float frontOpacity = 0.6f;
				/** 縁の明るさ */
				float edgeGlow = 1.8f;
				/** 光が当たっていないときの明るさ */
				float brightness = 0.9f;
				/** 光が当たっているときの明るさ */
				float litBrightness = 1.6f;
			};


		public:
			/** @brief 初期化（モデルは必要になったときに読み込む） */
			void Init(const Config& config);
			/** @brief ステージに合わせて置く（毎フレーム、ステージの更新のあとに呼ぶ） */
			void Update(const Stage& stage);
			/** @brief 描く（半透明。ほかのモデルを全部 Draw したあと、光より前に呼ぶ） */
			void Draw(RenderContext& rc);


		public:
			ColorGlassRenderer() = default;
			ColorGlassRenderer(const ColorGlassRenderer&) = delete;
			ColorGlassRenderer& operator=(const ColorGlassRenderer&) = delete;


		private:
			Config m_config;
			render::ColorBoxRenderer m_boxes;
			bool m_isInited = false;
		};
	}
}
