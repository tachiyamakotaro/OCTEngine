/**
 * @file MenuGuide.cpp
 * @brief 画面の右下に出す操作の案内
 */
#include "stdafx.h"
#include "MenuGuide.h"


namespace app
{
	namespace ui
	{
		namespace
		{
			// ---- 画像（ポーズ画面と同じ物） ----
			const char* const BUTTON_A_PATH = "Assets/sprite/pause/button_a.dds";
			const char* const BUTTON_B_PATH = "Assets/sprite/pause/button_b.dds";
			constexpr float BUTTON_PIXEL_W = 104.0f;
			constexpr float BUTTON_PIXEL_H = 102.0f;
			const char* const LABEL_OK_PATH = "Assets/sprite/pause/label_ok.dds";
			const char* const LABEL_BACK_PATH = "Assets/sprite/pause/label_back.dds";
			constexpr float LABEL_PIXEL_H = 306.0f;
			constexpr float LABEL_OK_PIXEL_W = 540.0f;
			constexpr float LABEL_BACK_PIXEL_W = 925.0f;

			// ---- 大きさ ----
			constexpr float ICON_SIZE = 56.0f;
			constexpr float LABEL_HEIGHT = 40.0f;
			/** ボタンの絵と文字の間 */
			constexpr float LABEL_GAP = 10.0f;
			/** 「Ⓐ OK」と「Ⓑ BACK」の間 */
			constexpr float PAIR_GAP = 36.0f;

			// ---- 置き方（画面の真ん中が原点、上が +Y） ----
			/** ウィンドウの右端と、上下の真ん中 */
			constexpr float WINDOW_RIGHT_X = 900.0f;
			constexpr float WINDOW_CENTER_Y = -470.0f;
			/** ウィンドウの高さと、左右の余白 */
			constexpr float WINDOW_HEIGHT = 84.0f;
			constexpr float WINDOW_PADDING_X = 30.0f;
		}


		MenuGuide::MenuGuide()
			: m_hasOk(true)
			, m_shape()
			, m_buttonAX(0.0f)
			, m_labelOkX(0.0f)
			, m_buttonBX(0.0f)
			, m_labelBackX(0.0f)
			, m_window()
			, m_buttonA()
			, m_labelOk()
			, m_buttonB()
			, m_labelBack()
		{}


		MenuGuide::~MenuGuide()
		{}


		void MenuGuide::Init(const MenuGuideButtons buttons)
		{
			m_hasOk = buttons == MenuGuideButtons::OkAndBack;
			// ボタンの絵は縁なし（もともと縁のある絵）、文字は縁を付ける
			m_buttonA.Init(BUTTON_A_PATH, BUTTON_PIXEL_W, BUTTON_PIXEL_H, ICON_SIZE);
			m_labelOk.InitWithOutline(LABEL_OK_PATH, LABEL_OK_PIXEL_W, LABEL_PIXEL_H, LABEL_HEIGHT);
			m_buttonB.Init(BUTTON_B_PATH, BUTTON_PIXEL_W, BUTTON_PIXEL_H, ICON_SIZE);
			m_labelBack.InitWithOutline(LABEL_BACK_PATH, LABEL_BACK_PIXEL_W, LABEL_PIXEL_H, LABEL_HEIGHT);
			Layout();
			m_window.Init(m_shape);
			Place(UIGroupTransform::Identity());
		}


		void MenuGuide::Place(const UIGroupTransform& group)
		{
			// 案内のウィンドウの真ん中を中心に、拡大縮小する
			const UIGroupTransform local = group.WithPivot(m_shape.center);
			const float y = m_shape.center.y;
			m_window.Place(local);
			m_buttonA.Place(m_buttonAX, y, 1.0f, menu_color::White, 0.0f, local);
			m_labelOk.Place(m_labelOkX, y, 1.0f, menu_color::White, 0.0f, local);
			m_buttonB.Place(m_buttonBX, y, 1.0f, menu_color::White, 0.0f, local);
			m_labelBack.Place(m_labelBackX, y, 1.0f, menu_color::White, 0.0f, local);
		}


		void MenuGuide::Draw(RenderContext& rc)
		{
			m_window.Draw(rc);
			if (m_hasOk)
			{
				m_buttonA.Draw(rc);
				m_labelOk.Draw(rc);
			}
			m_buttonB.Draw(rc);
			m_labelBack.Draw(rc);
		}


		void MenuGuide::Layout()
		{
			// 中身の幅（左から Ⓐ OK　Ⓑ BACK）
			const float okWidth = m_buttonA.width + LABEL_GAP + m_labelOk.width;
			const float backWidth = m_buttonB.width + LABEL_GAP + m_labelBack.width;
			const float contentWidth = m_hasOk ? okWidth + PAIR_GAP + backWidth : backWidth;

			// ウィンドウは右端をそろえる
			const float windowWidth = contentWidth + WINDOW_PADDING_X * 2.0f;
			m_shape.center = Vector2(WINDOW_RIGHT_X - windowWidth * 0.5f, WINDOW_CENTER_Y);
			m_shape.size = Vector2(windowWidth, WINDOW_HEIGHT);
			m_shape.headerHeight = 0.0f;

			// 左から順に並べたときの真ん中の X
			float left = WINDOW_RIGHT_X - windowWidth + WINDOW_PADDING_X;
			if (m_hasOk)
			{
				m_buttonAX = left + m_buttonA.width * 0.5f;
				left += m_buttonA.width + LABEL_GAP;
				m_labelOkX = left + m_labelOk.width * 0.5f;
				left += m_labelOk.width + PAIR_GAP;
			}
			m_buttonBX = left + m_buttonB.width * 0.5f;
			left += m_buttonB.width + LABEL_GAP;
			m_labelBackX = left + m_labelBack.width * 0.5f;
		}
	}
}
