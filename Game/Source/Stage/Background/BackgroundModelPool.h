#pragma once
#include <memory>
#include <string>
#include <vector>


/**
 * @file BackgroundModelPool.h
 * @brief 同じモデル・同じシェーダーで描く物を、数を変えながら毎フレーム置き直すための入れ物
 */
namespace app
{
	namespace stage
	{
		/**
		 * @brief モデルの読み込みの設定
		 */
		struct BackgroundModelSetting
		{
			/** モデル */
			std::string tkmFilePath;
			/** シェーダー */
			std::string fxFilePath;
			/** ピクセルシェーダーの関数名 */
			std::string psEntryPointFunc;
			/** モデルの上方向 */
			EnModelUpAxis upAxis = enModelUpAxisY;
			/** シェーダーに渡す値（b1）。Model がこの場所を覚えていて、描くたびに GPU に送る（入れ物より長生きさせる） */
			void* constants = nullptr;
			/** シェーダーに渡す値の大きさ */
			int constantsSize = 0;
		};


		/**
		 * @brief 同じモデルを何個も置く入れ物
		 * @details ・Model の読み込みは重いので、足りなくなったときだけ増やし、余った分は描かないだけにする（StageRenderer と同じ作り）
		 *          ・毎フレーム Clear → Place を置く数だけ → Draw の順に呼ぶ
		 *          ・ModelRender は model.fx 固定なので、Model を直接持っている
		 */
		class BackgroundModelPool
		{
		public:
			BackgroundModelPool() = default;
			~BackgroundModelPool() = default;

			/** 読み込みの設定を覚える（モデルは Place で足りなくなったときに読み込む） */
			void Init(const BackgroundModelSetting& setting);
			/** 置いた物を全部片付ける（毎フレームの最初に呼ぶ） */
			void Clear() { m_count = 0; }
			/** 1つ置く */
			void Place(const Vector3& position, const Vector3& scale);
			/** 置いた物を描く物として登録する */
			void Draw();
			/** 今フレーム置いた数（確認用） */
			size_t GetCount() const { return m_count; }

		private:
			/** 読み込みの設定 */
			BackgroundModelSetting m_setting;
			/** モデル（置いた数だけ使う） */
			std::vector<std::unique_ptr<Model>> m_models;
			/** 今フレーム置いた数 */
			size_t m_count = 0;
		};
	}
}
