/**
 * @file StageIntroUI.cpp
 * @brief ステージが始まる前の表示
 */
#include "stdafx.h"
#include "StageIntroUI.h"
#include "Source/Stage/StageGimmickTypes.h"


namespace app
{
	namespace ui
	{
		namespace
		{
			/** 背景（SceneManager のフェードと同じ画像） */
			const char* const BACKGROUND_PATH = "Assets/sprite/fade.dds";
			/** 「STAGE」の文字（544 x 192） */
			const char* const LABEL_PATH = "Assets/sprite/stage_intro/stage.dds";
			constexpr float LABEL_PIXEL_W = 544.0f;
			constexpr float LABEL_PIXEL_H = 192.0f;
			/** 「FINAL」の文字（489 x 192） */
			const char* const FINAL_PATH = "Assets/sprite/stage_intro/final.dds";
			constexpr float FINAL_PIXEL_W = 489.0f;
			constexpr float FINAL_PIXEL_H = 192.0f;
			/** 数字（132 x 192。どの数字も同じ大きさ） */
			const char* const DIGIT_PATHS[10] = {
				"Assets/sprite/stage_intro/digit_0.dds", "Assets/sprite/stage_intro/digit_1.dds",
				"Assets/sprite/stage_intro/digit_2.dds", "Assets/sprite/stage_intro/digit_3.dds",
				"Assets/sprite/stage_intro/digit_4.dds", "Assets/sprite/stage_intro/digit_5.dds",
				"Assets/sprite/stage_intro/digit_6.dds", "Assets/sprite/stage_intro/digit_7.dds",
				"Assets/sprite/stage_intro/digit_8.dds", "Assets/sprite/stage_intro/digit_9.dds",
			};
			constexpr float DIGIT_PIXEL_W = 132.0f;
			constexpr float DIGIT_PIXEL_H = 192.0f;

			/** 文字の高さ（画面は 1920 x 1080） */
			constexpr float TEXT_HEIGHT = 110.0f;
			/** 「STAGE」と右の物（数字・FINAL）の間（文字の高さに対する割合） */
			constexpr float LABEL_GAP_RATE = 0.12f;
			/** 数字どうしの間隔（数字の幅に対する割合。画像には左右に余白があるので少し詰める） */
			constexpr float DIGIT_STEP_RATE = 0.78f;
			/** 文字の中心の高さ（画面の真ん中が 0、上が +） */
			constexpr float TEXT_Y = 150.0f;

			// ---- ギミックのアイコン ----
			/** アイコンの画像の大きさ（256 x 256） */
			constexpr float ICON_PIXEL_SIZE = 256.0f;
			/** 画面に出す大きさ */
			constexpr float ICON_SIZE = 140.0f;
			/** となりのアイコンとの間（中心から中心） */
			constexpr float ICON_SPACING = 175.0f;
			/** アイコンの中心の高さ（画面の真ん中が 0、上が +） */
			constexpr float ICON_Y = -40.0f;

			/** そのアイコンの "type" のどれかが、ステージに置いてあるか */
			bool IsIconShown(const GimmickIcon& icon, const stage::StageGimmickTypes& types)
			{
				for (const char* type : icon.types)
				{
					if (types.Contains(type))
					{
						return true;
					}
				}
				return false;
			}
		}


		StageIntroUI::StageIntroUI()
			: m_background()
			, m_label()
			, m_final()
			, m_digits()
			, m_digitCount(0)
			, m_isFinal(false)
			, m_icons()
			, m_iconCount(0)
		{}


		StageIntroUI::~StageIntroUI()
		{}


		void StageIntroUI::InitNumber(const int stageNumber)
		{
			InitCommon();

			// 番号を上のけたから数字に分ける（1〜99 に収める）
			const int number = stageNumber < 1 ? 1 : (stageNumber > 99 ? 99 : stageNumber);
			m_digitCount = number >= 10 ? 2 : 1;
			int rest = number;
			for (int i = m_digitCount - 1; i >= 0; --i)
			{
				m_digits[i].Init(DIGIT_PATHS[rest % 10], DIGIT_PIXEL_W, DIGIT_PIXEL_H, TEXT_HEIGHT);
				rest /= 10;
			}

			// 数字の全体の幅（最後の1文字だけはまるごとの幅）
			const float digitWidth = m_digits[0].width;
			const float digitStep = digitWidth * DIGIT_STEP_RATE;
			const float valueWidth = digitStep * static_cast<float>(m_digitCount - 1) + digitWidth;
			float x = LayoutLabel(valueWidth) + digitWidth * 0.5f;
			for (int i = 0; i < m_digitCount; ++i)
			{
				m_digits[i].Place(x, TEXT_Y, 1.0f, menu_color::White);
				x += digitStep;
			}
		}


		void StageIntroUI::InitFinal()
		{
			InitCommon();
			m_isFinal = true;
			m_final.Init(FINAL_PATH, FINAL_PIXEL_W, FINAL_PIXEL_H, TEXT_HEIGHT);
			const float left = LayoutLabel(m_final.width);
			m_final.Place(left + m_final.width * 0.5f, TEXT_Y, 1.0f, menu_color::White);
		}


		void StageIntroUI::InitGimmickIcons(const stage::StageGimmickTypes& types)
		{
			// 表の上から順に、置いてあるギミックのアイコンだけ読み込む
			m_iconCount = 0;
			for (const GimmickIcon& icon : GIMMICK_ICON_TABLE)
			{
				if (IsIconShown(icon, types))
				{
					m_icons[m_iconCount].Init(icon.imagePath, ICON_PIXEL_SIZE, ICON_PIXEL_SIZE, ICON_SIZE);
					++m_iconCount;
				}
			}
			// 横に並べて、全体を画面の真ん中に
			const float left = -ICON_SPACING * static_cast<float>(m_iconCount - 1) * 0.5f;
			for (int i = 0; i < m_iconCount; ++i)
			{
				m_icons[i].Place(left + ICON_SPACING * static_cast<float>(i), ICON_Y, 1.0f, menu_color::White);
			}
		}


		void StageIntroUI::Render(RenderContext& rc)
		{
			m_background.Draw(rc);
			for (int i = 0; i < m_iconCount; ++i)
			{
				m_icons[i].Draw(rc);
			}
			m_label.Draw(rc);
			if (m_isFinal)
			{
				m_final.Draw(rc);
				return;
			}
			for (int i = 0; i < m_digitCount; ++i)
			{
				m_digits[i].Draw(rc);
			}
		}


		void StageIntroUI::InitCommon()
		{
			m_background.InitFullScreen(BACKGROUND_PATH);
			m_background.Place(0.0f, 0.0f, 1.0f, menu_color::White);
			m_label.Init(LABEL_PATH, LABEL_PIXEL_W, LABEL_PIXEL_H, TEXT_HEIGHT);
			m_digitCount = 0;
			m_isFinal = false;
		}


		float StageIntroUI::LayoutLabel(const float valueWidth)
		{
			// 全体の幅 = STAGE + 間 + 右の物
			const float gap = TEXT_HEIGHT * LABEL_GAP_RATE;
			const float totalWidth = m_label.width + gap + valueWidth;
			const float left = -totalWidth * 0.5f;
			m_label.Place(left + m_label.width * 0.5f, TEXT_Y, 1.0f, menu_color::White);
			return left + m_label.width + gap;
		}
	}
}
