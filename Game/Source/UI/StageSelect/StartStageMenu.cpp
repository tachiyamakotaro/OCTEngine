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

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	/**
	 * ウィンドウ（真ん中・幅と高さ・見出しの帯の高さ）
	 * 中身（ひし形の印から CONTINUE の右の「STAGE 10」まで）が左右の余白 70 くらいで収まる大きさ
	 */
	const app::ui::MenuWindowShape WINDOW_SHAPE = { Vector2(0.0f, 70.0f), Vector2(900.0f, 560.0f), 130.0f };
	/** 見出し（帯の真ん中に置く） */
	constexpr float HEADER_HEIGHT = 80.0f;
	/** 項目（Item の順に上から。帯の下の真ん中に、上下の間をそろえて並べる） */
	constexpr float ROW_PITCH = 110.0f;
	constexpr float ROW_HEIGHT = 64.0f;
	/**
	 * 項目の左端を、一番長い STAGE SELECT が真ん中に来る所から、この分だけずらす
	 * （CONTINUE の右に「STAGE 〇」が出るぶん右が長いので、少し左に寄せて全体をウィンドウの真ん中に）
	 */
	constexpr float CONTENT_OFFSET_X = -20.0f;
	/**
	 * CONTINUE の右に出す「STAGE 〇」の高さと、CONTINUE との間
	 * （stage_intro の画像は項目の画像より上下の余白が多いので、そのぶん大きめ。文字の高さで項目の約 7 割）
	 */
	constexpr float CONTINUE_STAGE_HEIGHT = 54.0f;
	constexpr float CONTINUE_STAGE_GAP = 30.0f;
	/** ひし形の印の大きさと、項目の左端からの距離 */
	constexpr float CURSOR_SIZE = 28.0f;
	constexpr float CURSOR_GAP = 40.0f;

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
			m_window.Init(WINDOW_SHAPE);
			// 文字には縁を付ける（ひし形の印はウィンドウの中なので縁なし）
			m_header.InitWithOutline(HEADER_PATH, HEADER_PIXEL_W, HEADER_PIXEL_H, HEADER_HEIGHT);
			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				m_items[i].InitWithOutline(ITEM_IMAGES[i].path, ITEM_IMAGES[i].pixelWidth, ITEM_PIXEL_H, ROW_HEIGHT);
			}
			m_continueStage.Init(CONTINUE_STAGE_HEIGHT);
			m_cursor.Init(CURSOR_PATH, 64.0f, 64.0f, CURSOR_SIZE);
			m_guide.Init(MenuGuideButtons::OkAndBack);
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
			// ウィンドウ → 中身 → 案内の順
			m_window.Draw(rc);
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
			m_guide.Draw(rc);
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
			// 左端は一番長い STAGE SELECT に合わせる（全体がだいたいウィンドウの真ん中に来る）
			const float left = WINDOW_SHAPE.center.x + CONTENT_OFFSET_X - m_items[static_cast<int>(Item::StageSelect)].width * 0.5f;
			const float pulse = 1.0f + PULSE_SCALE * std::sin(m_timer * PULSE_SPEED * 2.0f * math::PI);

			for (int i = 0; i < ITEM_COUNT; ++i)
			{
				const float scale = i == m_index ? pulse : 1.0f;
				UISprite& item = m_items[i];
				item.Place(left + item.width * scale * 0.5f, GetRowY(i), scale, GetItemColor(i));
			}

			// CONTINUE の右に、続きのステージ
			const int continueRow = static_cast<int>(Item::Continue);
			const float continueScale = m_index == continueRow ? pulse : 1.0f;
			const float stageLeft = left + m_items[continueRow].width * continueScale + CONTINUE_STAGE_GAP;
			m_continueStage.PlaceLeft(stageLeft, GetRowY(continueRow), continueScale, GetItemColor(continueRow));

			// 選んでいる項目の左にひし形の印
			m_cursor.Place(left - CURSOR_GAP, GetRowY(m_index), 1.0f, menu_color::Selected, 45.0f);
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
			// 見出しはウィンドウの帯の真ん中
			m_header.Place(WINDOW_SHAPE.center.x, WINDOW_SHAPE.GetHeaderCenterY(), 1.0f, menu_color::Selected);
		}


		float StartStageMenu::GetRowY(const int index) const
		{
			// 帯の下の真ん中を中心に、ROW_PITCH ずつ並べる（項目の数が変わっても真ん中にそろう）
			const float centerOffset = (static_cast<float>(ITEM_COUNT) - 1.0f) * 0.5f - static_cast<float>(index);
			return WINDOW_SHAPE.GetBodyCenterY() + ROW_PITCH * centerOffset;
		}
	}
}
