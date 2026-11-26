/**
 * @file TitleMenu.cpp
 * @brief タイトルメニュー
 */
#include "stdafx.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Sound/SoundManager.h"
#include "Source/Stage/StageCatalog.h"
#include "TitleMenu.h"
#include <cmath>


namespace
{
	// ---- 画像（パスと、表示するときの縦横の比を決める大きさ） ----
	struct ButtonImage
	{
		const char* path;
		float pixelWidth;
		float pixelHeight;
	};
	/**
	 * app::ui::TitleMenu::Item の順
	 * 大きさは画像そのものの大きさ（文字のまわりで切ってあり、縦は全部同じ）。
	 * 画像と比がちがうと文字が伸び縮みするので、画像を作り直したら Tools/make_title_result_sprites.py が出す大きさに合わせる
	 */
	constexpr ButtonImage BUTTON_IMAGES[] =
	{
		{ "Assets/sprite/title/button_start.dds",    1181.0f, 316.0f },
		{ "Assets/sprite/title/button_sound.dds",    1187.0f, 316.0f },
		{ "Assets/sprite/title/button_quit.dds",      799.0f, 316.0f },
	};
	const char* const CURSOR_PATH = "Assets/sprite/title/block.dds";

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	/**
	 * @brief ボタンの並べ方（横に並べる）
	 * @details ボタンの幅がちがうので、ボタンどうしのすき間（BUTTON_GAP）がそろい、全体が画面の真ん中に来る位置を
	 *          TitleMenu::LayoutItems で計算する（ボタンを増やしても、ここは直さなくてよい）
	 */
	constexpr float BUTTON_GAP = 120.0f;
	constexpr float BUTTON_Y = -380.0f;
	/** ボタンの高さ（文字の上下の余白も含む。文字の高さはこの約 8 割） */
	constexpr float BUTTON_HEIGHT = 56.0f;
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
	 * @brief タイトルの背景（遺跡の絵）は暗いので、ボタンは白い文字（地は透明）にして、色をかけて出す
	 *        選んでいる項目はロゴと同じ黄色、選んでいない項目は明るい灰色。ひし形の印もロゴと同じ黄色
	 */
	const Vector4 TITLE_COLOR_SELECTED(1.0f, 0.82f, 0.05f, 1.0f);
	const Vector4 TITLE_COLOR_NORMAL(0.82f, 0.82f, 0.86f, 1.0f);
	const Vector4 TITLE_COLOR_CURSOR(1.0f, 0.80f, 0.0f, 1.0f);

	// ---- タイトルのロゴ（大きさは画像そのものの大きさ。make_title_scene_sprites.py が出す大きさに合わせる） ----
	const char* const LOGO_PATH = "Assets/sprite/title/logo.dds";
	constexpr float LOGO_PIXEL_WIDTH = 1344.0f;
	constexpr float LOGO_PIXEL_HEIGHT = 720.0f;
	/** 画面に出す高さ（光の余白込み。文字の高さはこの約 7 割） */
	constexpr float LOGO_HEIGHT = 350.0f;
	/** 置く所 */
	constexpr float LOGO_X = 0.0f;
	constexpr float LOGO_Y = 300.0f;
	/** 開いてから出始めるまでの時間と、出きるまでの時間（秒） */
	constexpr float LOGO_APPEAR_DELAY = 0.2f;
	constexpr float LOGO_APPEAR_TIME = 1.0f;
	/** 出てくるときの大きさ（ここから 1 へ縮む） */
	constexpr float LOGO_APPEAR_SCALE = 1.08f;
	/** 出たあと、上下にゆれる幅と速さ（1秒に何回） */
	constexpr float LOGO_BOB_HEIGHT = 5.0f;
	constexpr float LOGO_BOB_SPEED = 0.35f;

	// ---- ボタンの後ろの暗い帯（画面の下の端から、この高さまで） ----
	const char* const BUTTON_SHADE_PATH = "Assets/sprite/title/menu_shade.dds";
	constexpr float BUTTON_SHADE_HEIGHT = 380.0f;
	constexpr float BUTTON_SHADE_Y = -static_cast<float>(FRAME_BUFFER_H) * 0.5f + BUTTON_SHADE_HEIGHT * 0.5f;
}


namespace app
{
	namespace ui
	{
		void TitleMenu::Init()
		{
			// ボタンの文字とひし形の印は、3D の背景の上に直接出すので縁を付ける
			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				const ButtonImage& image = BUTTON_IMAGES[i];
				m_items[i].InitWithOutline(image.path, image.pixelWidth, image.pixelHeight, BUTTON_HEIGHT);
			}
			m_cursor.InitWithOutline(CURSOR_PATH, 64.0f, 64.0f, CURSOR_SIZE);
			LayoutItems();

			// ロゴと、ボタンの後ろの帯（帯は縦のグラデーションだけなので、画面の幅いっぱいに伸ばしてよい）
			m_logo.Init(LOGO_PATH, LOGO_PIXEL_WIDTH, LOGO_PIXEL_HEIGHT, LOGO_HEIGHT);
			m_buttonShade.Init(BUTTON_SHADE_PATH, static_cast<float>(FRAME_BUFFER_W), BUTTON_SHADE_HEIGHT, BUTTON_SHADE_HEIGHT);
			m_buttonShade.Place(0.0f, BUTTON_SHADE_Y, 1.0f, menu_color::White);

			// ほかの画面を開いている間の、後ろの画面（ぼかしと暗さ）
			m_backdrop.Init();

			// サウンドオプション画面（ポーズ画面と共通）
			m_soundOption.Init();
			// START の中（どこから遊ぶかを選ぶ画面）と、ステージセレクト画面（全部のステージの札を作る）
			m_startStageMenu.Init();
			m_stageSelect.Init();
		}


		void TitleMenu::Open(const std::optional<int>& continueStage)
		{
			m_page = Page::Main;
			m_continueStage = continueStage;
			m_index = static_cast<int>(Item::Start);
			m_isDecided = false;
			m_timer = 0.0f;
			m_openTime = 0.0f;
			m_input.Reset();
			UpdateMainVisual();
			UpdateLogoVisual();
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
				else if (m_page == Page::StartStage)
				{
					result = UpdateStartStage();
				}
				else if (m_page == Page::StageSelect)
				{
					result = UpdateStageSelect();
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
			const float deltaTime = g_gameTime->GetFrameDeltaTime();
			m_timer += deltaTime;
			m_openTime += deltaTime;
			// ほかの画面を開いている間は、後ろの 3D の絵をぼかす（閉じたら少しずつ戻る）
			m_backdrop.Update(m_page != Page::Main);
			// ボタンとロゴは、後ろの画面の強さに合わせて薄くする（ほかの画面を開いている間も動かす）
			UpdateMainVisual();
			UpdateLogoVisual();
		}


		TitleMenu::Result TitleMenu::UpdateMain()
		{
			// 横に並べているので左右で選ぶ。縦に並べ替えても使えるように上下でも動かす
			if (m_input.IsLeft() || m_input.IsUp())
			{
				MoveCursor(-1);
			}
			else if (m_input.IsRight() || m_input.IsDown())
			{
				MoveCursor(1);
			}

			if (!m_input.IsDecide())
			{
				return Result::None;
			}

			SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
			switch (GetSelectedItem())
			{
			case Item::Start:
				// どこから遊ぶかを選ぶ（NEW GAME / CONTINUE / STAGE SELECT）
				m_page = Page::StartStage;
				m_startStageMenu.Open(m_continueStage);
				return Result::None;

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


		TitleMenu::Result TitleMenu::UpdateStartStage()
		{
			switch (m_startStageMenu.Update(m_input))
			{
			case StartStageMenu::Result::NewGame:
				return DecideStartStage(stage::FIRST_STAGE_INDEX, Result::StartGame);

			case StartStageMenu::Result::Continue:
				// CONTINUE は続きの記録があるときしか選べない（念のため、なければ最初のステージ）
				return DecideStartStage(m_continueStage.value_or(stage::FIRST_STAGE_INDEX), Result::ContinueGame);

			case StartStageMenu::Result::StageSelect:
				// 最初は続きのステージ（なければ最初のステージ）を選んでおく
				m_page = Page::StageSelect;
				m_stageSelect.Open(m_continueStage.value_or(stage::FIRST_STAGE_INDEX));
				return Result::None;

			case StartStageMenu::Result::Cancel:
				m_page = Page::Main;
				return Result::None;

			default:
				return Result::None;
			}
		}


		TitleMenu::Result TitleMenu::UpdateStageSelect()
		{
			switch (m_stageSelect.Update(m_input))
			{
			case StageSelectMenu::Result::Decide:
				// 選んだステージから、新しいプレイとして遊ぶ
				return DecideStartStage(m_stageSelect.GetSelectedStage(), Result::StartGame);

			case StageSelectMenu::Result::Cancel:
				// START の中に戻る（STAGE SELECT を選んだまま）
				m_page = Page::StartStage;
				return Result::None;

			default:
				return Result::None;
			}
		}


		TitleMenu::Result TitleMenu::DecideStartStage(const int stageIndex, const Result result)
		{
			// タイトルメニューに戻して、START を点滅させる（扉が開いていくのが見えるように）
			m_page = Page::Main;
			m_isDecided = true;
			m_timer = 0.0f;
			m_startStage = stageIndex;
			return result;
		}


		void TitleMenu::UpdateMainVisual()
		{
			// ほかの画面のウィンドウを開いている間は薄くして消す（ウィンドウの後ろで文字が重ならないように）
			const float mainAlpha = 1.0f - m_backdrop.GetRate();
			m_buttonShade.Place(0.0f, BUTTON_SHADE_Y, 1.0f, Vector4(1.0f, 1.0f, 1.0f, mainAlpha));

			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				const bool isSelected = i == m_index;
				if (!isSelected)
				{
					Vector4 color = TITLE_COLOR_NORMAL;
					color.w *= mainAlpha;
					m_items[i].Place(m_itemX[i], BUTTON_Y, 1.0f, color);
					continue;
				}
				float scale = SELECTED_SCALE;
				Vector4 color = TITLE_COLOR_SELECTED;
				color.w *= mainAlpha;
				if (m_isDecided)
				{
					// 決定したら点滅させる
					const bool isVisible = std::fmod(m_timer * BLINK_SPEED, 1.0f) < 0.5f;
					color.w *= isVisible ? 1.0f : 0.2f;
				}
				else
				{
					// 選んでいる間はゆっくり脈打つ
					scale += PULSE_SCALE * std::sin(m_timer * PULSE_SPEED * 2.0f * math::PI);
				}
				m_items[i].Place(m_itemX[i], BUTTON_Y, scale, color);
			}

			// 選んでいる項目の左にひし形の印
			const UISprite& selected = m_items[m_index];
			const float cursorX = m_itemX[m_index] - selected.width * SELECTED_SCALE * 0.5f - CURSOR_GAP;
			Vector4 cursorColor = TITLE_COLOR_CURSOR;
			cursorColor.w *= mainAlpha;
			m_cursor.Place(cursorX, BUTTON_Y, 1.0f, cursorColor, 45.0f);
		}


		void TitleMenu::MoveCursor(const int step)
		{
			// 端の次は反対の端（全部いつでも選べる）
			m_index = (m_index + step + ITEM_COUNT) % ITEM_COUNT;
			m_timer = 0.0f;
			SoundManager::Get().PlaySE(enSoundKind_CursorMove);
		}


		void TitleMenu::LayoutItems()
		{
			// 全体の幅 = ボタンの幅の合計 + すき間
			float totalWidth = BUTTON_GAP * static_cast<float>(ITEM_COUNT - 1);
			for (const UISprite& item : m_items)
			{
				totalWidth += item.width;
			}
			float left = -totalWidth * 0.5f;
			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				m_itemX[i] = left + m_items[i].width * 0.5f;
				left += m_items[i].width + BUTTON_GAP;
			}
		}


		void TitleMenu::UpdateLogoVisual()
		{
			// 少し大きい所から縮みながら出てくる
			const float rate = math::SmoothStep((m_openTime - LOGO_APPEAR_DELAY) / LOGO_APPEAR_TIME);
			const float scale = math::Lerp(LOGO_APPEAR_SCALE, 1.0f, rate);
			// 出きったあとは、ゆっくり上下にゆれる
			const float bob = std::sin(m_openTime * LOGO_BOB_SPEED * 2.0f * math::PI) * LOGO_BOB_HEIGHT * rate;
			// ほかの画面のウィンドウを開いている間は薄くして消す
			const float alpha = rate * (1.0f - m_backdrop.GetRate());
			m_logo.Place(LOGO_X, LOGO_Y + bob, scale, Vector4(1.0f, 1.0f, 1.0f, alpha));
		}


		void TitleMenu::Render(RenderContext& rc)
		{
			// 帯 → ロゴ → ボタンの順（ボタンがいちばん手前）
			m_buttonShade.Draw(rc);
			m_logo.Draw(rc);
			for (UISprite& item : m_items)
			{
				item.Draw(rc);
			}
			m_cursor.Draw(rc);
			// 後ろの画面を暗くする黒（3D の絵のぼかしは RenderingEngine がする）
			m_backdrop.Draw(rc);
			// ほかの画面は、ウィンドウに入れてタイトルメニューの上に重ねる
			if (m_page == Page::Sound)
			{
				m_soundOption.Render(rc);
			}
			else if (m_page == Page::StartStage)
			{
				m_startStageMenu.Render(rc);
			}
			else if (m_page == Page::StageSelect)
			{
				m_stageSelect.Render(rc);
			}
		}
	}
}
