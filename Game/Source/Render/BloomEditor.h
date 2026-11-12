#pragma once


/**
 * @file BloomEditor.h
 * @brief ブルーム（明るい所から光があふれる効果）を調整する ImGui（デバッグウィンドウの「Bloom」の中身）
 */
namespace app
{
	namespace render
	{
		/**
		 * @brief ブルームを調整する ImGui
		 * @details ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 */
		class BloomEditor
		{
		public:
			BloomEditor() = default;
			~BloomEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param renderingEngine ブルームをかけているエンジン
			 */
			void Draw(RenderingEngine& renderingEngine);
		};
	}
}
