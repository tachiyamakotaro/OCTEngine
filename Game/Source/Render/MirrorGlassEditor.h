#pragma once


/**
 * @file MirrorGlassEditor.h
 * @brief 奥の鏡の半透明ガラス（MirrorGlass）を調整する ImGui（デバッグウィンドウの「Display → Mirror Glass」の中身）
 */
namespace app
{
	namespace render
	{
		class MirrorGlass;


		/**
		 * @brief 奥の鏡のガラスを調整する ImGui
		 * @details ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 */
		class MirrorGlassEditor
		{
		public:
			MirrorGlassEditor() = default;
			~MirrorGlassEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param glass 調整するガラス
			 */
			void Draw(MirrorGlass& glass);
		};
	}
}
