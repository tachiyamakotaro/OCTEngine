/**
 * @file TitleMenu.cpp
 * @brief タイトルメニュー
 */
#include "stdafx.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Sound/SoundManager.h"
#include "TitleMenu.h"
#include <cmath>


namespace
{
	// ---- 画像（パスと元の大きさ。Tools/make_text_sprite.py が最後に表示する大きさ） ----
	struct ButtonImage
	{
		const char* path;
		float pixelWidth;
		float pixelHeight;
	};
	/** app::ui::TitleMenu::Item の順 */
	constexpr ButtonImage BUTTON_IMAGES[] =
	{
		{ "Assets/sprite/title/button_start.dds", 1046.0f, 256.0f },
		{ "Assets/sprite/title/button_sound.dds", 1052.0f, 256.0f },
		{ "Assets/sprite/title/button_quit.dds",   698.0f, 256.0f },
	};
	const char* const CURSOR_PATH = "Assets/sprite/title/block.dds";

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	/**
	 * @brief ボタンの中心（app::ui::TitleMenu::Item の順）
	 * @details 横に並べている。ボタンの幅がちがうので、ボタンどうしのすき間（約150）がそろい、3つ全体が画面の真ん中に来る位置にしている。
	 *          縦に並べたいときは X を全部 0 にして、Y をずらせばよい（入力は上下左右どちらでも動く）
	 */
	constexpr float BUTTON_X[] = { -424.0f, 55.0f, 478.0f };
	constexpr float BUTTON_Y[] = { -380.0f, -380.0f, -380.0f };
	/** ボタンの高さ（文字の上下の余白も含む） */
	constexpr float BUTTON_HEIGHT = 80.0f;
	/** ひし形の印の大きさと、ボタンの左端からの距離 */
	constexpr float CURSOR_SIZE = 26.0f;
	constexpr float CURSOR_GAP = 34.0f;

	// ---- 見た目 ----
	/** 選んでいる項目の大きさ */
	constexpr float SELECTED_SCALE = 1.12f;
	/** 選んでいる項目が脈打つ大きさと速さ（1秒に何回） */
	constexpr float PULSE_SCALE = 0.03f;
	constexpr float PULSE_SPEED = 0.8f;
	/** 決定したあとに点滅する速さ（1秒に何回） */
	constexpr float BLINK_SPEED = 8.0f;
	/**
	 * @brief タイトルの背景は白いので、ポーズ画面（暗い背景）とは別の色にしている
	 *        選んでいる項目は濃い色、選んでいない項目は薄い灰色。ひし形の印はロゴと同じ黄色
	 */
	const Vector4 TITLE_COLOR_SELECTED(1.0f, 0.82f, 0.05f, 1.0f);
	const Vector4 TITLE_COLOR_NORMAL(0.70f, 0.70f, 0.72f, 1.0f);
	const Vector4 TITLE_COLOR_CURSOR(1.0f, 0.80f, 0.0f, 1.0f);
}


namespace app
{
	namespace ui
	{
		void TitleMenu::Init()
		{
			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				const ButtonImage& image = BUTTON_IMAGES[i];
				m_items[i].Init(image.path, image.pixelWidth, image.pixelHeight, BUTTON_HEIGHT);
			}
			m_cursor.Init(CURSOR_PATH, 64.0f, 64.0f, CURSOR_SIZE);

			// サウンドオプション画面（ポーズ画面と共通）
			m_soundOption.Init();
		}


		void TitleMenu::Open()
		{
			m_page = Page::Main;
			m_index = static_cast<int>(Item::Start);
			m_isDecided = false;
			m_timer = 0.0f;
			m_input.Reset();
			UpdateMainVisual();
		}


		TitleMenu::Result TitleMenu::Update()
		{
			Result result = Result::None;
			// 決定したあとは入力を受け付けない（点滅だけ）
			if (!m_isDecided)
			{
				m_input.Update();
				if (m_page == Page::Main)
				{
					result = UpdateMain();
				}
				else if (m_soundOption.Update(m_input))
				{
					// サウンドオプション：B で閉じたらタイトルメニューに戻る
					m_page = Page::Main;
				}
			}
			Animate();
			return result;
		}


		void TitleMenu::Animate()
		{
			m_timer += g_gameTime->GetFrameDeltaTime();
			if (m_page == Page::Main)
			{
				UpdateMainVisual();
			}
		}


		TitleMenu::Result TitleMenu::UpdateMain()
		{
			// 横に並べているので左右で選ぶ。縦に並べ替えても使えるように上下でも動かす
			if (m_input.IsLeft() || m_input.IsUp())
			{
				m_index = (m_index + ITEM_COUNT - 1) % ITEM_COUNT;
				m_timer = 0.0f;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}
			else if (m_input.IsRight() || m_input.IsDown())
			{
				m_index = (m_index + 1) % ITEM_COUNT;
				m_timer = 0.0f;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}

			if (!m_input.IsDecide())
			{
				return Result::None;
			}

			SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
			switch (GetSelectedItem())
			{
			case Item::Start:
				m_isDecided = true;
				m_timer = 0.0f;
				return Result::StartGame;

			case Item::Quit:
				m_isDecided = true;
				m_timer = 0.0f;
				return Result::QuitGame;

			case Item::Sound:
			default:
				m_page = Page::Sound;
				m_soundOption.Open();
				return Result::None;
			}
		}


		void TitleMenu::UpdateMainVisual()
		{
			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				const bool isSelected = i == m_index;
				if (!isSelected)
				{
					m_items[i].Place(BUTTON_X[i], BUTTON_Y[i], 1.0f, TITLE_COLOR_NORMAL);
					continue;
				}
				float scale = SELECTED_SCALE;
				Vector4 color = TITLE_COLOR_SELECTED;
				if (m_isDecided)
				{
					// 決定したら点滅させる
					const bool isVisible = std::fmod(m_timer * BLINK_SPEED, 1.0f) < 0.5f;
					color.w = isVisible ? 1.0f : 0.2f;
				}
				else
				{
					// 選んでいる間はゆっくり脈打つ
					scale += PULSE_SCALE * std::sin(m_timer * PULSE_SPEED * 2.0f * math::PI);
				}
				m_items[i].Place(BUTTON_X[i], BUTTON_Y[i], scale, color);
			}

			// 選んでいる項目の左にひし形の印
			const UISprite& selected = m_items[m_index];
			const float cursorX = BUTTON_X[m_index] - selected.width * SELECTED_SCALE * 0.5f - CURSOR_GAP;
			m_cursor.Place(cursorX, BUTTON_Y[m_index], 1.0f, TITLE_COLOR_CURSOR, 45.0f);
		}


		void TitleMenu::Render(RenderContext& rc)
		{
			for (UISprite& item : m_items)
			{
				item.Draw(rc);
			}
			m_cursor.Draw(rc);
			// サウンドオプションはタイトルメニューの上に重ねる（半透明の黒い背景ごと）
			if (m_page == Page::Sound)
			{
				m_soundOption.Render(rc);
			}
		}
	}
}
