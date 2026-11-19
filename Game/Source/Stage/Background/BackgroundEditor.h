#pragma once


/**
 * @file BackgroundEditor.h
 * @brief ステージの奥の背景を調整する ImGui（デバッグウィンドウの「Background」の中身）
 */
namespace app
{
	namespace stage
	{
		class BackgroundRenderer;
		struct BackgroundLayerStyle;
		struct BackgroundTorchStyle;
		struct BackgroundStyle;


		/**
		 * @brief ステージの奥の背景を調整する ImGui
		 * @details ・BackgroundRenderer は ImGui を使わない。ImGui の項目はこのクラスにまとめる
		 *            （シーンに書くのは何もない。InGameEditor が Draw を1行呼ぶだけ）
		 *          ・ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 *          ・項目ごとの関数には、その調整値（BackgroundLayerStyle など）だけを渡す
		 */
		class BackgroundEditor
		{
		public:
			BackgroundEditor() = default;
			~BackgroundEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param renderer 背景の描画
			 */
			void Draw(BackgroundRenderer& renderer);

		private:
			/** 全体（色・並べ方） */
			void DrawCommon(BackgroundStyle& style);
			/**
			 * @brief 層1つぶん
			 * @param hasScale 大きさを変えられる層か（いちばん奥の壁は大きさを使わない）
			 */
			void DrawLayer(const char* label, BackgroundLayerStyle& style, bool hasScale);
			/** 松明1種類ぶん */
			void DrawTorch(const char* label, BackgroundTorchStyle& style);
		};
	}
}
