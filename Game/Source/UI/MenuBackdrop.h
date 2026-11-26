/**
 * @file MenuBackdrop.h
 * @brief ウィンドウを開いている間の、後ろの画面（3D の絵をぼかして、少し暗くする）
 * @details サウンドオプションやステージセレクトのウィンドウを開いている間、後ろの 3D の絵をぼかして、
 *          ウィンドウの中の文字に目が行くようにする。
 *          ・ぼかしは RenderingEngine のスクリーンのぼかし（GetScreenBlurPower）を使う（2D の画像はぼけない）
 *          ・ぼかしと暗さは、開く・閉じるときに少しずつ強く・弱くなる
 *          ・GetRate（0 〜 1）で、後ろのメニュー（タイトルのボタンなど）を薄くできる
 *          ・壊れるときに、スクリーンのぼかしを 0 に戻す（次のシーンがぼけたままにならないように）
 */
#pragma once
#include "Source/UI/UISprite.h"


namespace app
{
	namespace ui
	{
		/**
		 * @brief ウィンドウの後ろの画面
		 */
		class MenuBackdrop
		{
		public:
			MenuBackdrop();
			/** スクリーンのぼかしを 0 に戻す */
			~MenuBackdrop();
			MenuBackdrop(const MenuBackdrop&) = delete;
			MenuBackdrop& operator=(const MenuBackdrop&) = delete;

			/** 画像を読み込む */
			void Init();
			/**
			 * @brief ぼかしと暗さを進める（毎フレーム呼ぶ）
			 * @param isShown ウィンドウを開いているか（開いている間はだんだん強く、閉じたらだんだん弱く）
			 */
			void Update(bool isShown);
			/** 暗くする黒を描く（後ろのメニューのあと、ウィンドウの前に呼ぶ） */
			void Draw(RenderContext& rc);

			/** ぼかしと暗さの強さ（0 = なし 〜 1 = 開ききった） */
			float GetRate() const { return m_rate; }


		private:
			/** 今の強さを、スクリーンのぼかしと黒の濃さにする */
			void Apply();


		private:
			/** 暗くする黒（フェードの黒い画像を半透明にして使う） */
			UISprite m_dim;
			/** 強さ（0 〜 1） */
			float m_rate;
		};
	}
}
