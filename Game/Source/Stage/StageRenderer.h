/**
 * @file StageRenderer.h
 * @brief ステージの地形をモデルで描く
 * @details 今あるアセットは「黄色い床」（ground.tkm：50000×50000 の平らな板）だけなので、
 *          それを各ブロックの上面に、ブロックの幅 × 奥行きに縮めて置いている。
 *          奥の鏡があるときは、鏡に映った床も描く（鏡の面で反転したコピーを置く）。
 *
 *          立方体のモデルができたら、Config の modelPath / modelSize / isPlane を変えれば
 *          ブロック全体（前面・側面込み）を描けるようになる。
 *          扉・光・反射鏡などはまだ StageDebugOverlay で描いている。
 */
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		class Stage;


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
			 * @brief 矩形1つぶんのモデルを置く
			 * @param isReflected 鏡に映った像として置くか
			 * @param planeZ 鏡の面のZ座標
			 */
			void Place(ModelRender& model, const math::Rect& rect, bool isReflected, float planeZ);
			/**
			 * @brief 必要な数だけモデルを用意する
			 * @param models 用意する先
			 * @param cullMode カリング（鏡に映った像は表裏が逆になるので FRONT）
			 */
			void Reserve(std::vector<std::unique_ptr<ModelRender>>& models, size_t count, D3D12_CULL_MODE cullMode);


		private:
			/** 設定 */
			Config m_config;
			/** モデル（ブロックの数だけ使う） */
			std::vector<std::unique_ptr<ModelRender>> m_models;
			/** 鏡に映ったモデル（ブロックの数だけ使う） */
			std::vector<std::unique_ptr<ModelRender>> m_reflectionModels;
			/** 今フレーム描くモデルの数 */
			size_t m_activeCount = 0;
			/** 今フレーム描く鏡に映ったモデルの数 */
			size_t m_activeReflectionCount = 0;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
