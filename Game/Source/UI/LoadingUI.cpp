/**
 * @file LoadingUI.cpp
 * @brief ロード中の表示
 */
#include "stdafx.h"
#include "LoadingUI.h"
#include <cmath>


namespace app
{
	namespace ui
	{
		namespace
		{
			/** 「NOW LOADING」の文字（665 x 128） */
			const char* const LABEL_PATH = "Assets/sprite/loading/now_loading.dds";
			constexpr float LABEL_PIXEL_W = 665.0f;
			/** 点（52 x 128。文字と同じ高さの画像に、文字の下にそろえた点が1つ入っている） */
			const char* const DOT_PATH = "Assets/sprite/loading/dot.dds";
			constexpr float DOT_PIXEL_W = 52.0f;
			/** どちらの画像も高さは同じ */
			constexpr float TEXT_PIXEL_H = 128.0f;

			/** 画面に出す文字の高さ（画面は 1920 x 1080） */
			constexpr float TEXT_HEIGHT = 48.0f;
			/** 画面の右端・下端からの余白 */
			constexpr float MARGIN_RIGHT = 60.0f;
			constexpr float MARGIN_BOTTOM = 50.0f;
			/** 文字と1つ目の点の間（点の幅に対する割合。マイナスで詰める） */
			constexpr float DOT_GAP_RATE = -0.15f;
			/** 点どうしの間隔（点の幅に対する割合。画像には左右に余白があるので少し詰める） */
			constexpr float DOT_STEP_RATE = 0.7f;
			/** 点が1つ増えるまでの時間（秒）。0 → 1 → 2 → 3 → 0 … と繰り返す */
			constexpr float DOT_INTERVAL = 0.3f;
			/** この濃さより薄いときは描かない */
			constexpr float MIN_VISIBLE_OPACITY = 0.001f;
		}


		LoadingUI::LoadingUI()
			: m_label()
			, m_dots()
			, m_timer(0.0f)
			, m_opacity(0.0f)
		{}


		LoadingUI::~LoadingUI()
		{}


		void LoadingUI::Init()
		{
			m_label.Init(LABEL_PATH, LABEL_PIXEL_W, TEXT_PIXEL_H, TEXT_HEIGHT);
			for (UISprite& dot : m_dots)
			{
				dot.Init(DOT_PATH, DOT_PIXEL_W, TEXT_PIXEL_H, TEXT_HEIGHT);
			}
			Place(0);
		}


		void LoadingUI::Update(const float opacity)
		{
			m_opacity = opacity;
			if (m_opacity < MIN_VISIBLE_OPACITY)
			{
				// 出していない間はタイマーを戻す（次に出すときは点 0 個から始める）
				m_timer = 0.0f;
				return;
			}

			// 点の数を決める（0 → 1 → 2 → 3 → 0 …）
			const float cycle = DOT_INTERVAL * static_cast<float>(DOT_COUNT + 1);
			m_timer = std::fmod(m_timer + g_gameTime->GetFrameDeltaTime(), cycle);
			Place(static_cast<int>(m_timer / DOT_INTERVAL));
		}


		void LoadingUI::Place(const int shownDotCount)
		{
			// 右下にそろえて並べる。点は 3 つ分の場所をいつも空けておく（点が増えても文字が動かないように）
			const float dotWidth = m_dots[0].width;
			const float dotStep = dotWidth * DOT_STEP_RATE;
			const float dotsWidth = dotStep * static_cast<float>(DOT_COUNT - 1) + dotWidth;
			const float dotGap = dotWidth * DOT_GAP_RATE;
			const float totalWidth = m_label.width + dotGap + dotsWidth;

			// 座標は画面の真ん中が原点、上が +Y
			const float right = static_cast<float>(FRAME_BUFFER_W) * 0.5f - MARGIN_RIGHT;
			const float bottom = -static_cast<float>(FRAME_BUFFER_H) * 0.5f + MARGIN_BOTTOM;
			const float centerY = bottom + TEXT_HEIGHT * 0.5f;
			const float left = right - totalWidth;

			// 濃さを画像の透明度にする（まだ出さない点は透明）
			const Vector4 color(1.0f, 1.0f, 1.0f, m_opacity);
			const Vector4 hidden(1.0f, 1.0f, 1.0f, 0.0f);

			m_label.Place(left + m_label.width * 0.5f, centerY, 1.0f, color);
			float dotX = left + m_label.width + dotGap + dotWidth * 0.5f;
			for (int i = 0; i < DOT_COUNT; ++i)
			{
				m_dots[i].Place(dotX, centerY, 1.0f, i < shownDotCount ? color : hidden);
				dotX += dotStep;
			}
		}


		void LoadingUI::Render(RenderContext& rc)
		{
			if (m_opacity < MIN_VISIBLE_OPACITY)
			{
				return;
			}
			m_label.Draw(rc);
			for (UISprite& dot : m_dots)
			{
				dot.Draw(rc);
			}
		}
	}
}
