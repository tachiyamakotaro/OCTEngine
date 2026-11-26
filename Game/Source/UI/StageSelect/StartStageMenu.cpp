/**
 * @file StartStageMenu.cpp
 * @brief どこから遊ぶかを選ぶ画面
 */
#include "stdafx.h"
#include "StartStageMenu.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Sound/SoundManager.h"
#include "Source/UI/MenuInput.h"
#include <cmath>


namespace
{
	// ---- 画像（パスと元の大きさ） ----
	const char* const BACKGROUND_PATH = "Assets/sprite/fade.dds";
	/** 見出し（タイトルの START のボタンと同じ画像。白い文字に黄色をかける） */
	const char* const HEADER_PATH = "Assets/sprite/title/button_start.dds";
	constexpr float HEADER_PIXEL_W = 1181.0f;
	constexpr float HEADER_PIXEL_H = 316.0f;
	/**
	 * 項目の文字（Item の順。Tools/stage_select_image/make_stage_select_sprites.py で作った。高さは全部 316）
	 * 画像を作り直したら、スクリプトが出す大きさに合わせる
	 */
	struct ItemImage
	{
		const char* path;
		float pixelWidth;
	};
	constexpr ItemImage ITEM_IMAGES[] =
	{
		{ "Assets/sprite/stage_select/new_game.dds", 2243.0f },
		{ "Assets/sprite/stage_select/continue.dds", 2071.0f },
		{ "Assets/sprite/stage_select/title.dds",    2873.0f },
	};
	constexpr float ITEM_PIXEL_H = 316.0f;
	/** 印（白い四角。45 度回してひし形にする） */
	const char* const CURSOR_PATH = "Assets/sprite/title/block.dds";
	/** A / B の絵と、OK / BACK の文字（サウンドオプション画面と同じ物） */
	const char* const BUTTON_A_PATH = "Assets/sprite/pause/button_a.dds";
	const char* const BUTTON_B_PATH = "Assets/sprite/pause/button_b.dds";
	constexpr float BUTTON_AB_PIXEL_W = 104.0f;
	constexpr float BUTTON_AB_PIXEL_H = 102.0f;
	const char* const LABEL_OK_PATH = "Assets/sprite/pause/label_ok.dds";
	const char* const LABEL_BACK_PATH = "Assets/sprite/pause/label_back.dds";
	constexpr float LABEL_PIXEL_H = 306.0f;
	constexpr float LABEL_OK_PIXEL_W = 540.0f;
	constexpr float LABEL_BACK_PIXEL_W = 925.0f;

	/** 背景の濃さ */
	constexpr float BACKGROUND_ALPHA = 0.75f;

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	/** 見出し */
	constexpr float HEADER_Y = 270.0f;
	constexpr float HEADER_HEIGHT = 80.0f;
	/** 項目（Item の順に上から。左端をそろえて、一番長い STAGE SELECT が画面の真ん中に来るようにする） */
	constexpr float ROW_Y[] = { 100.0f, -10.0f, -120.0f };
	constexpr float ROW_HEIGHT = 64.0f;
	/**
	 * CONTINUE の右に出す「STAGE 〇」の高さと、CONTINUE との間
	 * （stage_intro の画像は項目の画像より上下の余白が多いので、そのぶん大きめ。文字の高さで項目の約 7 割）
	 */
	constexpr float CONTINUE_STAGE_HEIGHT = 54.0f;
	constexpr float CONTINUE_STAGE_GAP = 30.0f;
	/** ひし形の印の大きさと、項目の左端からの距離 */
	constexpr float CURSOR_SIZE = 28.0f;
	constexpr float CURSOR_GAP = 40.0f;
	/** 操作の案内（ステージセレクト画面と同じ位置） */
	constexpr float GUIDE_Y = -478.0f;
	constexpr float GUIDE_A_X = 400.0f;
	constexpr float GUIDE_B_X = 640.0f;
	constexpr float GUIDE_ICON_SIZE = 60.0f;
	constexpr float GUIDE_LABEL_HEIGHT = 40.0f;
	constexpr float GUIDE_LABEL_GAP = 10.0f;

	// ---- 見た目 ----
	/** 選んでいる項目が脈打つ大きさと速さ（1秒に何回） */
	constexpr float PULSE_SCALE = 0.03f;
	constexpr float PULSE_SPEED = 0.8f;
	/** 選べない項目（続きの記録がないときの CONTINUE）は薄くする */
	const Vector4 COLOR_DISABLED(0.75f, 0.75f, 0.78f, 0.3f);
}


namespace app
{
	namespace ui
	{
		StartStageMenu::StartStageMenu()
			: m_index(0)
			, m_canContinue(false)
			, m_timer(0.0f)
		{}


		StartStageMenu::~StartStageMenu()
		{}


		void StartStageMenu::Init()
		{
			m_background.InitFullScreen(BACKGROUND_PATH);
			m_header.Init(HEADER_PATH, HEADER_PIXEL_W, HEADER_PIXEL_H, HEADER_HEIGHT);
			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				m_items[i].Init(ITEM_IMAGES[i].path, ITEM_IMAGES[i].pixelWidth, ITEM_PIXEL_H, ROW_HEIGHT);
			}
			m_continueStage.Init(CONTINUE_STAGE_HEIGHT);
			m_cursor.Init(CURSOR_PATH, 64.0f, 64.0f, CURSOR_SIZE);
			m_buttonA.Init(BUTTON_A_PATH, BUTTON_AB_PIXEL_W, BUTTON_AB_PIXEL_H, GUIDE_ICON_SIZE);
			m_labelOk.Init(LABEL_OK_PATH, LABEL_OK_PIXEL_W, LABEL_PIXEL_H, GUIDE_LABEL_HEIGHT);
			m_buttonB.Init(BUTTON_B_PATH, BUTTON_AB_PIXEL_W, BUTTON_AB_PIXEL_H, GUIDE_ICON_SIZE);
			m_labelBack.Init(LABEL_BACK_PATH, LABEL_BACK_PIXEL_W, LABEL_PIXEL_H, GUIDE_LABEL_HEIGHT);
			PlaceFixedSprites();
		}


		void StartStageMenu::Open(const std::optional<int>& continueStage)
		{
			m_canContinue = continueStage.has_value();
			if (m_canContinue)
			{
				m_continueStage.SetStage(*continueStage);
			}
			m_index = static_cast<int>(m_canContinue ? Item::Continue : Item::NewGame);
			m_timer = 0.0f;
			UpdateVisual();
		}


		StartStageMenu::Result StartStageMenu::Update(const MenuInput& input)
		{
			m_timer += g_gameTime->GetFrameDeltaTime();

			// 縦に並べているので上下で選ぶ。ほかの画面と同じく左右でも動かす
			if (input.IsUp() || input.IsLeft())
			{
				MoveCursor(-1);
			}
			else if (input.IsDown() || input.IsRight())
			{
				MoveCursor(1);
			}

			if (input.IsDecide())
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
				switch (static_cast<Item>(m_index))
				{
				case Item::Continue:
					return Result::Continue;
				case Item::StageSelect:
					return Result::StageSelect;
				case Item::NewGame:
				default:
					return Result::NewGame;
				}
			}
			if (input.IsCancel())
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonBack);
				return Result::Cancel;
			}
			UpdateVisual();
			return Result::None;
		}


		void StartStageMenu::Render(RenderContext& rc)
		{
			m_background.Draw(rc);
			m_header.Draw(rc);
			for (UISprite& item : m_items)
			{
				item.Draw(rc);
			}
			// 続きのステージは、続きから遊べるときだけ
			if (m_canContinue)
			{
				m_continueStage.Draw(rc);
			}
			m_cursor.Draw(rc);
			m_buttonA.Draw(rc);
			m_labelOk.Draw(rc);
			m_buttonB.Draw(rc);
			m_labelBack.Draw(rc);
		}


		void StartStageMenu::MoveCursor(const int step)
		{
			// 選べる項目に当たるまで進める（NEW GAME はいつも選べる）
			int next = m_index;
			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				next = (next + step + ITEM_COUNT) % ITEM_COUNT;
				if (IsSelectable(next))
				{
					break;
				}
			}
			if (next == m_index)
			{
				return;
			}
			m_index = next;
			m_timer = 0.0f;
			SoundManager::Get().PlaySE(enSoundKind_CursorMove);
		}


		bool StartStageMenu::IsSelectable(const int index) const
		{
			if (index == static_cast<int>(Item::Continue))
			{
				return m_canContinue;
			}
			return true;
		}


		void StartStageMenu::UpdateVisual()
		{
			// 左端は一番長い STAGE SELECT に合わせる（全体がだいたい画面の真ん中に来る）
			const float left = -m_items[static_cast<int>(Item::StageSelect)].width * 0.5f;
			const float pulse = 1.0f + PULSE_SCALE * std::sin(m_timer * PULSE_SPEED * 2.0f * math::PI);

			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				const float scale = i == m_index ? pulse : 1.0f;
				UISprite& item = m_items[i];
				item.Place(left + item.width * scale * 0.5f, ROW_Y[i], scale, GetItemColor(i));
			}

			// CONTINUE の右に、続きのステージ
			const int continueRow = static_cast<int>(Item::Continue);
			const float continueScale = m_index == continueRow ? pulse : 1.0f;
			const float stageLeft = left + m_items[continueRow].width * continueScale + CONTINUE_STAGE_GAP;
			m_continueStage.PlaceLeft(stageLeft, ROW_Y[continueRow], continueScale, GetItemColor(continueRow));

			// 選んでいる項目の左にひし形の印
			m_cursor.Place(left - CURSOR_GAP, ROW_Y[m_index], 1.0f, menu_color::Selected, 45.0f);
		}


		Vector4 StartStageMenu::GetItemColor(const int index) const
		{
			if (!IsSelectable(index))
			{
				return COLOR_DISABLED;
			}
			return index == m_index ? menu_color::Selected : menu_color::Normal;
		}


		void StartStageMenu::PlaceFixedSprites()
		{
			m_background.Place(0.0f, 0.0f, 1.0f, Vector4(1.0f, 1.0f, 1.0f, BACKGROUND_ALPHA));
			m_header.Place(0.0f, HEADER_Y, 1.0f, menu_color::Selected);

			// 案内：A OK　B BACK（右下）
			m_buttonA.Place(GUIDE_A_X, GUIDE_Y, 1.0f, menu_color::White);
			m_labelOk.Place(GUIDE_A_X + m_buttonA.width * 0.5f + GUIDE_LABEL_GAP + m_labelOk.width * 0.5f, GUIDE_Y, 1.0f, menu_color::White);
			m_buttonB.Place(GUIDE_B_X, GUIDE_Y, 1.0f, menu_color::White);
			m_labelBack.Place(GUIDE_B_X + m_buttonB.width * 0.5f + GUIDE_LABEL_GAP + m_labelBack.width * 0.5f, GUIDE_Y, 1.0f, menu_color::White);
		}
	}
}
