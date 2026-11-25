/**
 * @file StageCard.cpp
 * @brief ステージセレクトに並べる、1ステージぶんの札
 */
#include "stdafx.h"
#include "StageCard.h"
#include "Source/Stage/StageCatalog.h"
#include "Source/Stage/StageGimmickTypes.h"
#include "Source/Stage/StageLoader.h"


namespace app
{
	namespace ui
	{
		namespace
		{
			// ---- 画像（StageIntroUI と同じ物） ----
			/** 札の枠と地（300 x 400 の白い角丸の四角。色を付けて使う） */
			const char* const CARD_PATH = "Assets/sprite/stage_select/card.dds";
			constexpr float CARD_PIXEL_W = 300.0f;
			constexpr float CARD_PIXEL_H = 400.0f;
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
			/** 数字どうしの間隔（数字の幅に対する割合。画像には左右に余白があるので少し詰める） */
			constexpr float DIGIT_STEP_RATE = 0.78f;
			/** アイコンの画像の大きさ（256 x 256） */
			constexpr float ICON_PIXEL_SIZE = 256.0f;

			// ---- 札の中の置き方（札の真ん中が原点、上が +Y。拡大率 1 のとき） ----
			/** 枠の太さ（枠は地より、この分だけ上下左右に大きい） */
			constexpr float FRAME_THICKNESS = 7.0f;
			/** 「STAGE」の文字 */
			constexpr float LABEL_HEIGHT = 46.0f;
			constexpr float LABEL_Y = 130.0f;
			/** 番号 */
			constexpr float DIGIT_HEIGHT = 150.0f;
			constexpr float NUMBER_Y = 15.0f;
			/** FINAL（数字より横に長いので小さめ） */
			constexpr float FINAL_HEIGHT = 92.0f;
			/** ギミックのアイコン */
			constexpr float ICON_SIZE = 50.0f;
			constexpr float ICON_SPACING = 56.0f;
			constexpr float ICON_Y = -125.0f;
			/** アイコンの列の左右の余白（アイコンが多いときは間を詰めて、この内側に収める） */
			constexpr float ICON_SIDE_MARGIN = 20.0f;

			// ---- 色 ----
			/** 地（暗い色。後ろのタイトル画面が少し透ける） */
			const Vector4 PANEL_COLOR(0.10f, 0.09f, 0.13f, 0.9f);
			/** 選んでいない札の枠 */
			const Vector4 FRAME_COLOR_NORMAL(0.45f, 0.45f, 0.50f, 0.9f);
			/** 選んでいない札のアイコン（少し暗くする） */
			const Vector4 ICON_COLOR_NORMAL(0.65f, 0.65f, 0.68f, 1.0f);
		}


		StageCard::StageCard()
			: m_frame()
			, m_panel()
			, m_label()
			, m_final()
			, m_digits()
			, m_digitOffsetX()
			, m_digitCount(0)
			, m_isFinal(false)
			, m_icons()
			, m_iconCount(0)
			, m_iconSpacing(ICON_SPACING)
		{}


		StageCard::~StageCard()
		{}


		void StageCard::Init(const int stageIndex)
		{
			// 枠と地（同じ白い角丸の四角。枠は少し大きくする。色は Place で付ける）
			m_frame.Init(CARD_PATH, CARD_PIXEL_W, CARD_PIXEL_H, HEIGHT + FRAME_THICKNESS * 2.0f);
			m_panel.Init(CARD_PATH, CARD_PIXEL_W, CARD_PIXEL_H, HEIGHT);
			m_label.Init(LABEL_PATH, LABEL_PIXEL_W, LABEL_PIXEL_H, LABEL_HEIGHT);

			// 最後のステージは「FINAL」、ほかは番号
			if (stage::IsFinalStage(stageIndex))
			{
				InitFinal();
			}
			else
			{
				InitNumber(stage::GetStageNumber(stageIndex));
			}
			InitIcons(stageIndex);
		}


		void StageCard::PlaceNormal(const Vector2& center)
		{
			Place(center, Look{ 1.0f, FRAME_COLOR_NORMAL, menu_color::Normal, ICON_COLOR_NORMAL });
		}


		void StageCard::PlaceSelected(const Vector2& center, const float scale)
		{
			Place(center, Look{ scale, menu_color::Selected, menu_color::Selected, menu_color::White });
		}


		void StageCard::Draw(RenderContext& rc)
		{
			m_frame.Draw(rc);
			m_panel.Draw(rc);
			m_label.Draw(rc);
			if (m_isFinal)
			{
				m_final.Draw(rc);
			}
			for (int i = 0; i < m_digitCount; ++i)
			{
				m_digits[i].Draw(rc);
			}
			for (int i = 0; i < m_iconCount; ++i)
			{
				m_icons[i].Draw(rc);
			}
		}


		void StageCard::InitNumber(const int stageNumber)
		{
			// 番号を上のけたから数字に分ける（1〜99 に収める）
			const int number = stageNumber < 1 ? 1 : (stageNumber > 99 ? 99 : stageNumber);
			m_digitCount = number >= 10 ? 2 : 1;
			int rest = number;
			for (int i = m_digitCount - 1; i >= 0; --i)
			{
				m_digits[i].Init(DIGIT_PATHS[rest % 10], DIGIT_PIXEL_W, DIGIT_PIXEL_H, DIGIT_HEIGHT);
				rest /= 10;
			}

			// 数字の全体が札の真ん中に来るように、1文字ずつの X を決める
			const float digitWidth = m_digits[0].width;
			const float digitStep = digitWidth * DIGIT_STEP_RATE;
			const float totalWidth = digitStep * static_cast<float>(m_digitCount - 1) + digitWidth;
			for (int i = 0; i < m_digitCount; ++i)
			{
				m_digitOffsetX[i] = -totalWidth * 0.5f + digitWidth * 0.5f + digitStep * static_cast<float>(i);
			}
			m_isFinal = false;
		}


		void StageCard::InitFinal()
		{
			m_final.Init(FINAL_PATH, FINAL_PIXEL_W, FINAL_PIXEL_H, FINAL_HEIGHT);
			m_digitCount = 0;
			m_isFinal = true;
		}


		void StageCard::InitIcons(const int stageIndex)
		{
			// そのステージの JSON の "type" を見て、表の上から順に、置いてあるギミックのアイコンだけ読み込む
			const stage::StageGimmickTypes types = stage::LoadStageGimmickTypes(stage::GetStageInfo(stageIndex));
			m_iconCount = 0;
			for (const GimmickIcon& icon : GIMMICK_ICON_TABLE)
			{
				if (icon.IsShownIn(types))
				{
					m_icons[m_iconCount].Init(icon.imagePath, ICON_PIXEL_SIZE, ICON_PIXEL_SIZE, ICON_SIZE);
					++m_iconCount;
				}
			}

			// アイコンが多くて札からはみ出るときは、間を詰める
			m_iconSpacing = ICON_SPACING;
			if (m_iconCount >= 2)
			{
				const float maxSpacing = (WIDTH - ICON_SIDE_MARGIN * 2.0f - ICON_SIZE) / static_cast<float>(m_iconCount - 1);
				m_iconSpacing = maxSpacing < ICON_SPACING ? maxSpacing : ICON_SPACING;
			}
		}


		void StageCard::Place(const Vector2& center, const Look& look)
		{
			const float s = look.scale;
			m_frame.Place(center.x, center.y, s, look.frameColor);
			m_panel.Place(center.x, center.y, s, PANEL_COLOR);
			m_label.Place(center.x, center.y + LABEL_Y * s, s, look.textColor);
			if (m_isFinal)
			{
				m_final.Place(center.x, center.y + NUMBER_Y * s, s, look.textColor);
			}
			for (int i = 0; i < m_digitCount; ++i)
			{
				m_digits[i].Place(center.x + m_digitOffsetX[i] * s, center.y + NUMBER_Y * s, s, look.textColor);
			}

			// アイコンは横に並べて、全体を札の真ん中に
			const float left = -m_iconSpacing * static_cast<float>(m_iconCount - 1) * 0.5f;
			for (int i = 0; i < m_iconCount; ++i)
			{
				const float offsetX = left + m_iconSpacing * static_cast<float>(i);
				m_icons[i].Place(center.x + offsetX * s, center.y + ICON_Y * s, s, look.iconColor);
			}
		}
	}
}
