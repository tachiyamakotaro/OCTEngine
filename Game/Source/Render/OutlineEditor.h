#pragma once


/**
 * @file OutlineEditor.h
 * @brief アウトライン（黒い縁）を調整する ImGui（デバッグウィンドウの「Outline」の中身）
 */
namespace app
{
	namespace render
	{
		/**
		 * @brief アウトラインを調整する ImGui
		 * @details ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 */
		class OutlineEditor
		{
		public:
			OutlineEditor() = default;
			~OutlineEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param renderingEngine アウトラインを描いているエンジン
			 */
			void Draw(RenderingEngine& renderingEngine);
		};
	}
}
