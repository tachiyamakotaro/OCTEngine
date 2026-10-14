/**
 * @file PauseMenu.cpp
 * @brief ポーズ画面
 */
#include "stdafx.h"
#include "PauseMenu.h"
#include "Source/Sound/SoundManager.h"


namespace
{
	// ---- 画像（パス） ----
	const char* const BACKGROUND_PATH = "Assets/sprite/fade.dds";
	const char* const PAUSE_TITLE_PATH = "Assets/sprite/pause/pause_title.dds";
	const char* const ITEM_SOUND_PATH = "Assets/sprite/pause/item_sound.dds";
	const char* const ITEM_TITLE_PATH = "Assets/sprite/pause/item_title.dds";
	const char* const BLOCK_PATH = "Assets/sprite/pause/block.dds";
	const char* const LABEL_OK_PATH = "Assets/sprite/pause/label_ok.dds";
	const char* const LABEL_BACK_PATH = "Assets/sprite/pause/label_back.dds";
	const char* const BUTTON_A_PATH = "Assets/sprite/pause/button_a.dds";
	const char* const BUTTON_B_PATH = "Assets/sprite/pause/button_b.dds";
	const char* const ITEM_QUIT_PATH = "Assets/sprite/pause/item_quit.dds";

	/** 背景の濃さ */
	constexpr float BACKGROUND_ALPHA = 0.65f;

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	constexpr float PAUSE_TITLE_Y = 260.0f;
	constexpr float PAUSE_TITLE_HEIGHT = 170.0f;
	constexpr float MAIN_ITEM_HEIGHT = 95.0f;

	constexpr float MAIN_ITEM_Y[] = { 40.0f, -100.0f ,-240.0f };
	static_assert(sizeof(MAIN_ITEM_Y) / sizeof(MAIN_ITEM_Y[0]) ==
		static_cast<size_t>(app::ui::PauseMenu::MainItem::Num), "MAIN_ITEM_Y の数を項目の数に合わせてください");

	constexpr float CURSOR_SIZE = 34.0f;
	constexpr float CURSOR_GAP = 50.0f;

	constexpr float GUIDE_Y = -430.0f;
	constexpr float GUIDE_ICON_SIZE = 80.0f;
	constexpr float GUIDE_LABEL_HEIGHT = 50.0f;

	/** 選んでいる項目の大きさ */
	constexpr float SELECTED_SCALE = 1.12f;
}


namespace app
{
	namespace ui
	{
		PauseMenu::PauseMenu()
			: m_page(Page::Main)
			, m_mainIndex(0)
		{}


		PauseMenu::~PauseMenu()
		{}


		void PauseMenu::Init()
		{
			// 画面いっぱいの背景（小さい黒い画像を引き伸ばす）
			m_background.InitFullScreen(BACKGROUND_PATH);
			m_pauseTitle.Init(PAUSE_TITLE_PATH, 973.0f, 235.0f, PAUSE_TITLE_HEIGHT);

			m_mainItems[static_cast<int>(MainItem::Sound)].Init(ITEM_SOUND_PATH, 1155.0f, 268.0f, MAIN_ITEM_HEIGHT);
			m_mainItems[static_cast<int>(MainItem::Title)].Init(ITEM_TITLE_PATH, 1025.0f, 265.0f, MAIN_ITEM_HEIGHT);
			m_mainItems[static_cast<int>(MainItem::Quit)].Init(ITEM_QUIT_PATH, 698.0f, 256.0f, MAIN_ITEM_HEIGHT);

			m_cursor.Init(BLOCK_PATH, 64.0f, 64.0f, CURSOR_SIZE);
			m_buttonA.Init(BUTTON_A_PATH, 128.0f, 128.0f, GUIDE_ICON_SIZE);
			m_buttonB.Init(BUTTON_B_PATH, 128.0f, 128.0f, GUIDE_ICON_SIZE);
			m_labelOk.Init(LABEL_OK_PATH, 171.0f, 96.0f, GUIDE_LABEL_HEIGHT);
			m_labelBack.Init(LABEL_BACK_PATH, 304.0f, 96.0f, GUIDE_LABEL_HEIGHT);

			// サウンドオプション画面（タイトルメニューと共通）
			m_soundOption.Init();

			// 動かない画像はここで置いておく
			m_background.Place(0.0f, 0.0f, 1.0f, Vector4(1.0f, 1.0f, 1.0f, BACKGROUND_ALPHA));
			m_pauseTitle.Place(0.0f, PAUSE_TITLE_Y, 1.0f, menu_color::White);
		}


		void PauseMenu::Open()
		{
			m_page = Page::Main;
			m_mainIndex = static_cast<int>(MainItem::Sound);
			m_input.Reset();
			UpdateMainVisual();
		}


		PauseMenu::Result PauseMenu::Update()
		{
			m_input.Update();

			// - ボタンはどの画面からでもポーズ解除
			if (g_pad[0]->IsTrigger(enButtonSelect))
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonBack);
				return Result::Resume;
			}

			if (m_page == Page::Main)
			{
				return UpdateMain();
			}
			// サウンドオプション：B で閉じたらポーズ画面に戻る
			if (m_soundOption.Update(m_input))
			{
				m_page = Page::Main;
				UpdateMainVisual();
			}
			return Result::None;
		}


		PauseMenu::Result PauseMenu::UpdateMain()
		{
			const int count = static_cast<int>(MainItem::Num);
			if (m_input.IsUp())
			{
				m_mainIndex = (m_mainIndex + count - 1) % count;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}
			else if (m_input.IsDown())
			{
				m_mainIndex = (m_mainIndex + 1) % count;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}
			UpdateMainVisual();

			if (m_input.IsCancel())
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonBack);
				return Result::Resume;
			}
			if (m_input.IsDecide())
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
				switch (GetSelectedMainItem())
				{
				case MainItem::Title:
					return Result::GoToTitle;

				case MainItem::Quit:
					return Result::QuitGame;

				case MainItem::Sound:
				default:
					// サウンドオプションへ
					m_page = Page::Sound;
					m_soundOption.Open();
					break;
				}
			}
			return Result::None;
		}


		void PauseMenu::UpdateMainVisual()
		{
			for (int i = 0; i < static_cast<int>(MainItem::Num); ++i)
			{
				const bool isSelected = i == m_mainIndex;
				m_mainItems[i].Place(0.0f, MAIN_ITEM_Y[i], isSelected ? SELECTED_SCALE : 1.0f, isSelected ? menu_color::Selected : menu_color::Normal);
			}
			// 選んでいる項目の左にひし形の印
			const UISprite& selected = m_mainItems[m_mainIndex];
			const float cursorX = -selected.width * SELECTED_SCALE * 0.5f - CURSOR_GAP;
			m_cursor.Place(cursorX, MAIN_ITEM_Y[m_mainIndex], 1.0f, menu_color::Selected, 45.0f);

			// 案内：A OK / B BACK（右下）
			m_buttonA.Place(430.0f, GUIDE_Y, 1.0f, menu_color::White);
			m_labelOk.Place(430.0f + GUIDE_ICON_SIZE * 0.5f + 10.0f + m_labelOk.width * 0.5f, GUIDE_Y, 1.0f, menu_color::White);
			m_buttonB.Place(640.0f, GUIDE_Y, 1.0f, menu_color::White);
			m_labelBack.Place(640.0f + GUIDE_ICON_SIZE * 0.5f + 10.0f + m_labelBack.width * 0.5f, GUIDE_Y, 1.0f, menu_color::White);
		}


		void PauseMenu::Render(RenderContext& rc)
		{
			if (m_page == Page::Sound)
			{
				// 背景はサウンドオプションが描く
				m_soundOption.Render(rc);
				return;
			}
			// 描いた順に重なる（背景がいちばん下）
			m_background.Draw(rc);
			m_pauseTitle.Draw(rc);
			for (UISprite& item : m_mainItems)
			{
				item.Draw(rc);
			}
			m_cursor.Draw(rc);
			m_buttonA.Draw(rc);
			m_labelOk.Draw(rc);
			m_buttonB.Draw(rc);
			m_labelBack.Draw(rc);
		}
	}
}
