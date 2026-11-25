/**
 * @file StageSelectMenu.cpp
 * @brief ステージセレクト画面
 */
#include "stdafx.h"
#include "StageSelectMenu.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Sound/SoundManager.h"
#include "Source/Stage/StageCatalog.h"
#include "Source/UI/MenuInput.h"
#include <cmath>


namespace
{
	// ---- 画像（パスと元の大きさ） ----
	const char* const BACKGROUND_PATH = "Assets/sprite/fade.dds";
	/** 見出し「STAGE SELECT」（白い文字。Tools/stage_select_image/make_stage_select_sprites.py で作った） */
	const char* const TITLE_PATH = "Assets/sprite/stage_select/title.dds";
	constexpr float TITLE_PIXEL_W = 2873.0f;
	constexpr float TITLE_PIXEL_H = 316.0f;
	/** ページの印（白い四角。45 度回してひし形にする） */
	const char* const MARK_PATH = "Assets/sprite/title/block.dds";
	/** LB / RB の絵（192 x 112） */
	const char* const BUTTON_LB_PATH = "Assets/sprite/prompt/button_lb.dds";
	const char* const BUTTON_RB_PATH = "Assets/sprite/prompt/button_rb.dds";
	constexpr float BUTTON_LR_PIXEL_W = 192.0f;
	constexpr float BUTTON_LR_PIXEL_H = 112.0f;
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
	constexpr float TITLE_Y = 390.0f;
	constexpr float TITLE_HEIGHT = 80.0f;
	/** 札の真ん中の高さと、となりの札との間（中心から中心） */
	constexpr float CARD_Y = 40.0f;
	constexpr float CARD_PITCH = app::ui::StageCard::WIDTH + 40.0f;
	/** ページの印 */
	constexpr float PAGE_MARK_Y = -255.0f;
	constexpr float PAGE_MARK_SPACING = 44.0f;
	constexpr float PAGE_MARK_SIZE = 18.0f;
	constexpr float PAGE_MARK_SIZE_CURRENT = 26.0f;
	/** LB / RB の絵（ページの印の両わき） */
	constexpr float BUTTON_LR_HEIGHT = 50.0f;
	constexpr float BUTTON_LR_GAP = 80.0f;
	/** 操作の案内（右下。後ろに透けて見えるタイトルのボタン（Y = -380）と重ならないように、少し下げている） */
	constexpr float GUIDE_Y = -478.0f;
	constexpr float GUIDE_A_X = 400.0f;
	constexpr float GUIDE_B_X = 640.0f;
	constexpr float GUIDE_ICON_SIZE = 60.0f;
	constexpr float GUIDE_LABEL_HEIGHT = 40.0f;
	constexpr float GUIDE_LABEL_GAP = 10.0f;

	// ---- 見た目 ----
	/** 選んでいる札の大きさと、脈打つ大きさ・速さ（1秒に何回） */
	constexpr float SELECTED_SCALE = 1.08f;
	constexpr float PULSE_SCALE = 0.02f;
	constexpr float PULSE_SPEED = 0.8f;
	/** 今のページではない印の色 */
	const Vector4 PAGE_MARK_COLOR_OTHER(0.45f, 0.45f, 0.50f, 1.0f);
}


namespace app
{
	namespace ui
	{
		StageSelectMenu::StageSelectMenu()
			: m_cursor(stage::STAGE_COUNT)
			, m_timer(0.0f)
		{}


		StageSelectMenu::~StageSelectMenu()
		{}


		void StageSelectMenu::Init()
		{
			m_background.InitFullScreen(BACKGROUND_PATH);
			m_title.Init(TITLE_PATH, TITLE_PIXEL_W, TITLE_PIXEL_H, TITLE_HEIGHT);

			// 札はステージの数だけ（StageCatalog の表を足す・減らすと、ここで作る数も変わる）
			m_cards.clear();
			m_cards.reserve(stage::STAGE_COUNT);
			for (int i = 0; i < stage::STAGE_COUNT; ++i)
			{
				auto card = std::make_unique<StageCard>();
				card->Init(i);
				m_cards.push_back(std::move(card));
			}

			// ページの印はページの数だけ（大きさは今のページかどうかで変えるので、Place で拡大率をかける）
			m_pageMarks.clear();
			for (int i = 0; i < m_cursor.GetPageCount(); ++i)
			{
				auto mark = std::make_unique<UISprite>();
				mark->Init(MARK_PATH, 64.0f, 64.0f, PAGE_MARK_SIZE);
				m_pageMarks.push_back(std::move(mark));
			}

			m_buttonLB.Init(BUTTON_LB_PATH, BUTTON_LR_PIXEL_W, BUTTON_LR_PIXEL_H, BUTTON_LR_HEIGHT);
			m_buttonRB.Init(BUTTON_RB_PATH, BUTTON_LR_PIXEL_W, BUTTON_LR_PIXEL_H, BUTTON_LR_HEIGHT);
			m_buttonA.Init(BUTTON_A_PATH, BUTTON_AB_PIXEL_W, BUTTON_AB_PIXEL_H, GUIDE_ICON_SIZE);
			m_labelOk.Init(LABEL_OK_PATH, LABEL_OK_PIXEL_W, LABEL_PIXEL_H, GUIDE_LABEL_HEIGHT);
			m_buttonB.Init(BUTTON_B_PATH, BUTTON_AB_PIXEL_W, BUTTON_AB_PIXEL_H, GUIDE_ICON_SIZE);
			m_labelBack.Init(LABEL_BACK_PATH, LABEL_BACK_PIXEL_W, LABEL_PIXEL_H, GUIDE_LABEL_HEIGHT);
			PlaceFixedSprites();
		}


		void StageSelectMenu::Open()
		{
			// 前に遊んだステージ（はじめは最初のステージ）を選んだ状態で開く
			m_cursor.Select(stage::GetCurrentStageIndex());
			m_timer = 0.0f;
			UpdateVisual();
		}


		StageSelectMenu::Result StageSelectMenu::Update(const MenuInput& input)
		{
			m_timer += g_gameTime->GetFrameDeltaTime();

			// 左右で1つずつ、LB / RB でページごと
			bool isMoved = false;
			if (input.IsLeft())
			{
				isMoved = m_cursor.Move(-1);
			}
			else if (input.IsRight())
			{
				isMoved = m_cursor.Move(1);
			}
			else if (input.IsPrevPage())
			{
				isMoved = m_cursor.TurnPage(-1);
			}
			else if (input.IsNextPage())
			{
				isMoved = m_cursor.TurnPage(1);
			}
			if (isMoved)
			{
				m_timer = 0.0f;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}

			if (input.IsDecide())
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
				return Result::Decide;
			}
			if (input.IsCancel())
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonBack);
				return Result::Cancel;
			}
			UpdateVisual();
			return Result::None;
		}


		void StageSelectMenu::Render(RenderContext& rc)
		{
			m_background.Draw(rc);
			m_title.Draw(rc);

			// 今のページの札だけ描く
			const StagePageRange page = m_cursor.GetPageRange();
			for (int i = 0; i < page.count; ++i)
			{
				m_cards[page.first + i]->Draw(rc);
			}

			// ページの印と LB / RB は、2ページ以上あるときだけ
			if (m_cursor.GetPageCount() >= 2)
			{
				for (auto& mark : m_pageMarks)
				{
					mark->Draw(rc);
				}
				m_buttonLB.Draw(rc);
				m_buttonRB.Draw(rc);
			}

			m_buttonA.Draw(rc);
			m_labelOk.Draw(rc);
			m_buttonB.Draw(rc);
			m_labelBack.Draw(rc);
		}


		void StageSelectMenu::UpdateVisual()
		{
			PlaceCards();
			PlacePageMarks();
		}


		void StageSelectMenu::PlaceCards()
		{
			// 最後のページが5個より少なくても、全体が画面の真ん中に来るようにする
			const StagePageRange page = m_cursor.GetPageRange();
			const int selected = m_cursor.GetSelectedStage();
			const float left = -CARD_PITCH * static_cast<float>(page.count - 1) * 0.5f;
			for (int i = 0; i < page.count; ++i)
			{
				const int stageIndex = page.first + i;
				const Vector2 center(left + CARD_PITCH * static_cast<float>(page.SlotOf(stageIndex)), CARD_Y);
				StageCard& card = *m_cards[stageIndex];
				if (stageIndex != selected)
				{
					card.PlaceNormal(center);
					continue;
				}
				// 選んでいる札はゆっくり脈打つ
				const float pulse = PULSE_SCALE * std::sin(m_timer * PULSE_SPEED * 2.0f * math::PI);
				card.PlaceSelected(center, SELECTED_SCALE + pulse);
			}
		}


		void StageSelectMenu::PlacePageMarks()
		{
			// 横に並べて、全体を画面の真ん中に。今のページは黄色で大きく
			const int pageCount = static_cast<int>(m_pageMarks.size());
			const int currentPage = m_cursor.GetPage();
			const float left = -PAGE_MARK_SPACING * static_cast<float>(pageCount - 1) * 0.5f;
			for (int i = 0; i < pageCount; ++i)
			{
				const bool isCurrent = i == currentPage;
				const float scale = isCurrent ? PAGE_MARK_SIZE_CURRENT / PAGE_MARK_SIZE : 1.0f;
				const Vector4& color = isCurrent ? menu_color::Selected : PAGE_MARK_COLOR_OTHER;
				m_pageMarks[i]->Place(left + PAGE_MARK_SPACING * static_cast<float>(i), PAGE_MARK_Y, scale, color, 45.0f);
			}
		}


		void StageSelectMenu::PlaceFixedSprites()
		{
			m_background.Place(0.0f, 0.0f, 1.0f, Vector4(1.0f, 1.0f, 1.0f, BACKGROUND_ALPHA));
			m_title.Place(0.0f, TITLE_Y, 1.0f, menu_color::Selected);

			// LB / RB はページの印の両わき（ページの数で印の幅が変わるので、それに合わせる）
			const int pageCount = static_cast<int>(m_pageMarks.size());
			const float marksHalfWidth = PAGE_MARK_SPACING * static_cast<float>(pageCount - 1) * 0.5f;
			const float buttonX = marksHalfWidth + BUTTON_LR_GAP + m_buttonLB.width * 0.5f;
			m_buttonLB.Place(-buttonX, PAGE_MARK_Y, 1.0f, menu_color::White);
			m_buttonRB.Place(buttonX, PAGE_MARK_Y, 1.0f, menu_color::White);

			// 案内：A OK　B BACK（右下）
			m_buttonA.Place(GUIDE_A_X, GUIDE_Y, 1.0f, menu_color::White);
			m_labelOk.Place(GUIDE_A_X + m_buttonA.width * 0.5f + GUIDE_LABEL_GAP + m_labelOk.width * 0.5f, GUIDE_Y, 1.0f, menu_color::White);
			m_buttonB.Place(GUIDE_B_X, GUIDE_Y, 1.0f, menu_color::White);
			m_labelBack.Place(GUIDE_B_X + m_buttonB.width * 0.5f + GUIDE_LABEL_GAP + m_labelBack.width * 0.5f, GUIDE_Y, 1.0f, menu_color::White);
		}
	}
}
