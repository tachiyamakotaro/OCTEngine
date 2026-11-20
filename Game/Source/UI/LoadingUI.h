/**
 * @file LoadingUI.h
 * @brief ロード中の表示（シーンを切り替える暗い画面の右下に「NOW LOADING」と、増えていく点「...」）
 * @details 見た目は全部画像（UISprite）。画像は Assets/sprite/loading/（now_loading.dds・dot.dds）。
 *          SceneManager が持っていて、シーンを切り替えている間だけ、フェードの暗さと同じ濃さで出す
 *          （暗くなるのに合わせて出てきて、明るくなるのに合わせて消える）。ゲームを終わるときの暗転では出さない。
 *          OCTEngine の読み込みは裏で行わないので、読み込んでいる間は画面が止まる。
 *          止まる直前のフレームでこの表示が出ているので、止まっている間も画面には「NOW LOADING」が残る。
 */
#pragma once
#include "Source/UI/UISprite.h"
#include <array>


namespace app
{
	namespace ui
	{
		/**
		 * @brief ロード中の表示
		 */
		class LoadingUI
		{
		public:
			LoadingUI();
			~LoadingUI();
			LoadingUI(const LoadingUI&) = delete;
			LoadingUI& operator=(const LoadingUI&) = delete;

			/** 画像を読み込んで並べる（SceneManager のコンストラクタで1回） */
			void Init();
			/**
			 * @brief 毎フレームの更新（点の数を増やし、濃さを決める）
			 * @param opacity 濃さ（0=出さない 〜 1=はっきり出す。フェードの暗さをそのまま渡す）
			 */
			void Update(float opacity);
			/** 描く（フェードの画像より後に呼ぶ） */
			void Render(RenderContext& rc);

		private:
			/** 点の数（「...」の 3 つ） */
			static constexpr int DOT_COUNT = 3;
			/**
			 * @brief 右下にそろえて並べ、今の濃さを付ける
			 * @param shownDotCount 出す点の数（0〜3）
			 */
			void Place(int shownDotCount);

		private:
			/** 「NOW LOADING」の文字 */
			UISprite m_label;
			/** 点（左から） */
			std::array<UISprite, DOT_COUNT> m_dots;
			/** 点を増やすためのタイマー（出していないときは 0 に戻す） */
			float m_timer;
			/** 今の濃さ */
			float m_opacity;
		};
	}
}
