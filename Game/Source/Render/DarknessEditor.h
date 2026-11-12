#pragma once


/**
 * @file DarknessEditor.h
 * @brief 暗がり（DarknessRenderer）の表示を切り替える ImGui（デバッグウィンドウの「Display → Darkness」の中身）
 */
namespace app
{
	namespace render
	{
		class DarknessRenderer;


		/**
		 * @brief 暗がりと手前の壁の表示を切り替える ImGui
		 * @details ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ。
		 *          スクリーンショットを撮るときに、暗がり・手前の壁を一時的に消すために使う（見た目だけ。ステージの JSON は変わらない）
		 */
		class DarknessEditor
		{
		public:
			DarknessEditor() = default;
			~DarknessEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param darkness 調整する暗がりの描画
			 */
			void Draw(DarknessRenderer& darkness);
		};
	}
}
