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
	/** 見出し「STAGE SELECT」（白い文字。Tools/stage_select_image/make_stage_select_sprites.py で作った） */
	const char* const TITLE_PATH = "Assets/sprite/stage_select/title.dds";
	constexpr float TITLE_PIXEL_W = 2873.0f;
	constexpr float TITLE_PIXEL_H = 316.0f;
	/** 今どこを選んでいるかの印（白い四角。45 度回してひし形にする） */
	const char* const MARK_PATH = "Assets/sprite/title/block.dds";
	/** LB / RB の絵（192 x 112） */
	const char* const BUTTON_LB_PATH = "Assets/sprite/prompt/button_lb.dds";
	const char* const BUTTON_RB_PATH = "Assets/sprite/prompt/button_rb.dds";
	constexpr float BUTTON_LR_PIXEL_W = 192.0f;
	constexpr float BUTTON_LR_PIXEL_H = 112.0f;

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	/**
	 * ウィンドウ（真ん中・幅と高さ・見出しの帯の高さ）
	 * 横に流れる札の、真ん中から2枚ずつ（と、消えかけの3枚目）が収まる幅
	 */
	const app::ui::MenuWindowShape WINDOW_SHAPE = { Vector2(0.0f, 70.0f), Vector2(1790.0f, 800.0f), 130.0f };
	/** 見出し（帯の真ん中に置く） */
	constexpr float TITLE_HEIGHT = 80.0f;
	/** 真ん中の札の真ん中の高さ（帯の下の真ん中から少し上。下にひし形を置くため） */
	const float CARD_Y = WINDOW_SHAPE.GetBodyCenterY() + 35.0f;
	/** 今どこを選んでいるかのひし形（ウィンドウの下の端の少し上） */
	const float STAGE_MARK_Y = WINDOW_SHAPE.GetBodyCenterY() - 250.0f;
	constexpr float STAGE_MARK_SPACING = 34.0f;
	constexpr float STAGE_MARK_SIZE = 14.0f;
	constexpr float STAGE_MARK_SIZE_CURRENT = 24.0f;
	/** LB / RB の絵（ひし形の列の両わき） */
	constexpr float BUTTON_LR_HEIGHT = 50.0f;
	constexpr float BUTTON_LR_GAP = 60.0f;

	// ---- 見た目 ----
	/** 選んでいる札が脈打つ大きさ・速さ（1秒に何回） */
	constexpr float PULSE_SCALE = 0.02f;
	constexpr float PULSE_SPEED = 0.8f;
	/** 今ではないひし形の色 */
	const Vector4 STAGE_MARK_COLOR_OTHER(0.45f, 0.45f, 0.50f, 1.0f);
	/** LB / RB を押したときに、絵をどれだけ大きくするかと、元に戻るまでの時間（秒） */
	constexpr float BUTTON_POP_SCALE = 0.35f;
	constexpr float BUTTON_POP_TIME = 0.25f;
	/** LB / RB を押してからの時間の初めの値（押していない = 戻りきっている） */
	constexpr float BUTTON_POP_IDLE = BUTTON_POP_TIME;
}


namespace app
{
	namespace ui
	{
		StageSelectMenu::StageSelectMenu()
			: m_cursor(stage::STAGE_COUNT)
			, m_carousel(Vector2(WINDOW_SHAPE.center.x, CARD_Y), stage::STAGE_COUNT)
			, m_timer(0.0f)
			, m_timeSinceLB(BUTTON_POP_IDLE)
			, m_timeSinceRB(BUTTON_POP_IDLE)
		{}


		StageSelectMenu::~StageSelectMenu()
		{}


		void StageSelectMenu::Init()
		{
			m_window.Init(WINDOW_SHAPE);
			// 見出しの文字には縁を付ける
			m_title.InitWithOutline(TITLE_PATH, TITLE_PIXEL_W, TITLE_PIXEL_H, TITLE_HEIGHT);

			// 札はステージの数だけ（StageCatalog の表を足す・減らすと、ここで作る数も変わる）
			m_cards.clear();
			m_cards.reserve(stage::STAGE_COUNT);
			for (int i = 0; i < stage::STAGE_COUNT; ++i)
			{
				auto card = std::make_unique<StageCard>();
				card->Init(i);
				m_cards.push_back(std::move(card));
			}

			// ひし形もステージの数だけ（大きさは今のステージかどうかで変えるので、Place で拡大率をかける）
			m_stageMarks.clear();
			for (int i = 0; i < stage::STAGE_COUNT; ++i)
			{
				auto mark = std::make_unique<UISprite>();
				mark->Init(MARK_PATH, 64.0f, 64.0f, STAGE_MARK_SIZE);
				m_stageMarks.push_back(std::move(mark));
			}

			m_buttonLB.Init(BUTTON_LB_PATH, BUTTON_LR_PIXEL_W, BUTTON_LR_PIXEL_H, BUTTON_LR_HEIGHT);
			m_buttonRB.Init(BUTTON_RB_PATH, BUTTON_LR_PIXEL_W, BUTTON_LR_PIXEL_H, BUTTON_LR_HEIGHT);
			m_guide.Init(MenuGuideButtons::OkAndBack);
			m_popup.Hide();
		}


		void StageSelectMenu::Open(const int initialStage)
		{
			m_cursor.Select(initialStage);
			// 開いたときは、すべらせずに選んでいる札を真ん中に
			m_carousel.SnapTo(m_cursor.GetSelectedStage());
			m_timer = 0.0f;
			m_timeSinceLB = BUTTON_POP_IDLE;
			m_timeSinceRB = BUTTON_POP_IDLE;
			m_popup.Open();
			UpdateVisual();
		}


		void StageSelectMenu::Close()
		{
			m_popup.Close();
		}


		void StageSelectMenu::Animate()
		{
			m_popup.Update();
			if (m_popup.IsVisible())
			{
				UpdateVisual();
			}
		}


		StageSelectMenu::Result StageSelectMenu::Update(const MenuInput& input)
		{
			const float deltaTime = g_gameTime->GetFrameDeltaTime();
			m_timer += deltaTime;
			m_timeSinceLB += deltaTime;
			m_timeSinceRB += deltaTime;

			// 左右で1つずつ、LB / RB で5つずつ
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
				// 押したことが分かるように、絵をポンとさせる
				m_timeSinceLB = 0.0f;
				isMoved = m_cursor.Jump(-1);
			}
			else if (input.IsNextPage())
			{
				m_timeSinceRB = 0.0f;
				isMoved = m_cursor.Jump(1);
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

			// 選んでいる札が真ん中に来るように、列を少しずつ流す
			m_carousel.Update(m_cursor.GetSelectedStage());
			UpdateVisual();
			return Result::None;
		}


		void StageSelectMenu::Render(RenderContext& rc)
		{
			if (!m_popup.IsVisible())
			{
				return;
			}
			// ウィンドウ → 中身 → 案内の順
			m_window.Draw(rc);
			m_title.Draw(rc);

			// 見えている札だけ描く。選んでいる札は最後（ほかの札より手前）
			const int selected = m_cursor.GetSelectedStage();
			for (int i = 0; i < static_cast<int>(m_cards.size()); ++i)
			{
				if (i != selected && m_carousel.IsVisible(i))
				{
					m_cards[i]->Draw(rc);
				}
			}
			m_cards[selected]->Draw(rc);

			for (auto& mark : m_stageMarks)
			{
				mark->Draw(rc);
			}
			m_buttonLB.Draw(rc);
			m_buttonRB.Draw(rc);

			m_guide.Draw(rc);
		}


		void StageSelectMenu::UpdateVisual()
		{
			// ウィンドウの真ん中を中心に、出し入れの拡大率・濃さをかける
			const UIGroupTransform group = m_popup.MakeTransform(WINDOW_SHAPE.center);
			m_window.Place(group);
			// 見出しはウィンドウの帯の真ん中
			m_title.Place(WINDOW_SHAPE.center.x, WINDOW_SHAPE.GetHeaderCenterY(), 1.0f, menu_color::Selected, 0.0f, group);
			PlaceCards(group);
			PlaceStageMarks(group);
			PlaceJumpButtons(group);
			// 案内は、案内のウィンドウの真ん中を中心に拡大縮小する
			m_guide.Place(group);
		}


		void StageSelectMenu::PlaceCards(const UIGroupTransform& group)
		{
			const int selected = m_cursor.GetSelectedStage();
			for (int i = 0; i < static_cast<int>(m_cards.size()); ++i)
			{
				// 横に流れる並べ方で決めた位置・大きさ・濃さに、ウィンドウの拡大率・濃さをかける
				StageCardPose pose = m_carousel.GetPose(i);
				pose.center = group.ApplyPosition(pose.center);
				pose.scale = group.ApplyScale(pose.scale);
				pose.alpha *= group.alpha;
				if (i != selected)
				{
					m_cards[i]->PlaceNormal(pose);
					continue;
				}
				// 選んでいる札はゆっくり脈打つ
				pose.scale += group.ApplyScale(PULSE_SCALE * std::sin(m_timer * PULSE_SPEED * 2.0f * math::PI));
				m_cards[i]->PlaceSelected(pose);
			}
		}


		void StageSelectMenu::PlaceStageMarks(const UIGroupTransform& group)
		{
			// 横に並べて、全体をウィンドウの真ん中に。今のステージは黄色で大きく
			const int markCount = static_cast<int>(m_stageMarks.size());
			const int selected = m_cursor.GetSelectedStage();
			const float left = WINDOW_SHAPE.center.x - STAGE_MARK_SPACING * static_cast<float>(markCount - 1) * 0.5f;
			for (int i = 0; i < markCount; ++i)
			{
				const bool isCurrent = i == selected;
				const float scale = isCurrent ? STAGE_MARK_SIZE_CURRENT / STAGE_MARK_SIZE : 1.0f;
				const Vector4& color = isCurrent ? menu_color::Selected : STAGE_MARK_COLOR_OTHER;
				m_stageMarks[i]->Place(left + STAGE_MARK_SPACING * static_cast<float>(i), STAGE_MARK_Y, scale, color, 45.0f, group);
			}
		}


		void StageSelectMenu::PlaceJumpButtons(const UIGroupTransform& group)
		{
			// LB / RB はひし形の列の両わき（ステージの数で列の幅が変わるので、それに合わせる）
			const int markCount = static_cast<int>(m_stageMarks.size());
			const float marksHalfWidth = STAGE_MARK_SPACING * static_cast<float>(markCount - 1) * 0.5f;
			const float buttonX = marksHalfWidth + BUTTON_LR_GAP + m_buttonLB.width * 0.5f;
			m_buttonLB.Place(WINDOW_SHAPE.center.x - buttonX, STAGE_MARK_Y, GetButtonPopScale(m_timeSinceLB), menu_color::White, 0.0f, group);
			m_buttonRB.Place(WINDOW_SHAPE.center.x + buttonX, STAGE_MARK_Y, GetButtonPopScale(m_timeSinceRB), menu_color::White, 0.0f, group);
		}


		float StageSelectMenu::GetButtonPopScale(const float timeSincePressed)
		{
			// 押した瞬間に大きくなって、BUTTON_POP_TIME かけてなめらかに戻る
			const float rate = math::SmoothStep(timeSincePressed / BUTTON_POP_TIME);
			return 1.0f + BUTTON_POP_SCALE * (1.0f - rate);
		}
	}
}
