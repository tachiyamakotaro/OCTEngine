/**
 * @file StageNameLabel.cpp
 * @brief 「STAGE 5」「STAGE FINAL」の文字
 */
#include "stdafx.h"
#include "StageNameLabel.h"
#include "Source/Stage/StageCatalog.h"


namespace app
{
	namespace ui
	{
		namespace
		{
			// ---- 画像（StageIntroUI と同じ物） ----
			const char* const LABEL_PATH = "Assets/sprite/stage_intro/stage.dds";
			constexpr float LABEL_PIXEL_W = 544.0f;
			constexpr float LABEL_PIXEL_H = 192.0f;
			const char* const FINAL_PATH = "Assets/sprite/stage_intro/final.dds";
			constexpr float FINAL_PIXEL_W = 489.0f;
			constexpr float FINAL_PIXEL_H = 192.0f;
			const char* const DIGIT_PATHS[10] = {
				"Assets/sprite/stage_intro/digit_0.dds", "Assets/sprite/stage_intro/digit_1.dds",
				"Assets/sprite/stage_intro/digit_2.dds", "Assets/sprite/stage_intro/digit_3.dds",
				"Assets/sprite/stage_intro/digit_4.dds", "Assets/sprite/stage_intro/digit_5.dds",
				"Assets/sprite/stage_intro/digit_6.dds", "Assets/sprite/stage_intro/digit_7.dds",
				"Assets/sprite/stage_intro/digit_8.dds", "Assets/sprite/stage_intro/digit_9.dds",
			};
			constexpr float DIGIT_PIXEL_W = 132.0f;
			constexpr float DIGIT_PIXEL_H = 192.0f;

			/** 「STAGE」と右の物（数字・FINAL）の間（文字の高さに対する割合） */
			constexpr float LABEL_GAP_RATE = 0.12f;
			/** 数字どうしの間隔（数字の幅に対する割合。画像には左右に余白があるので少し詰める） */
			constexpr float DIGIT_STEP_RATE = 0.78f;
		}


		StageNameLabel::StageNameLabel()
			: m_label()
			, m_final()
			, m_digits()
			, m_shownDigits()
			, m_digitCount(1)
			, m_isFinal(false)
		{}


		StageNameLabel::~StageNameLabel()
		{}


		void StageNameLabel::Init(const float height)
		{
			// メニューのほかの文字とそろえて、縁を付ける
			m_label.InitWithOutline(LABEL_PATH, LABEL_PIXEL_W, LABEL_PIXEL_H, height);
			m_final.InitWithOutline(FINAL_PATH, FINAL_PIXEL_W, FINAL_PIXEL_H, height);
			for (auto& digitKinds : m_digits)
			{
				for (int i = 0; i < DIGIT_KIND_COUNT; ++i)
				{
					digitKinds[i].InitWithOutline(DIGIT_PATHS[i], DIGIT_PIXEL_W, DIGIT_PIXEL_H, height);
				}
			}
		}


		void StageNameLabel::SetStage(const int stageIndex)
		{
			m_isFinal = stage::IsFinalStage(stageIndex);
			if (m_isFinal)
			{
				m_digitCount = 0;
				return;
			}
			// 番号を上のけたから数字に分ける（1〜99 に収める）
			const int stageNumber = stage::GetStageNumber(stageIndex);
			const int number = stageNumber < 1 ? 1 : (stageNumber > 99 ? 99 : stageNumber);
			m_digitCount = number >= 10 ? 2 : 1;
			int rest = number;
			for (int i = m_digitCount - 1; i >= 0; --i)
			{
				m_shownDigits[i] = rest % 10;
				rest /= 10;
			}
		}


		void StageNameLabel::PlaceLeft(const float left, const float y, const float scale, const Vector4& color)
		{
			// 「STAGE」
			const float labelWidth = m_label.width * scale;
			m_label.Place(left + labelWidth * 0.5f, y, scale, color);
			const float valueLeft = left + labelWidth + m_label.height * LABEL_GAP_RATE * scale;

			// 右に FINAL か数字
			if (m_isFinal)
			{
				m_final.Place(valueLeft + m_final.width * scale * 0.5f, y, scale, color);
				return;
			}
			const float digitWidth = m_digits[0][0].width * scale;
			float x = valueLeft + digitWidth * 0.5f;
			for (int i = 0; i < m_digitCount; ++i)
			{
				m_digits[i][m_shownDigits[i]].Place(x, y, scale, color);
				x += digitWidth * DIGIT_STEP_RATE;
			}
		}


		void StageNameLabel::Draw(RenderContext& rc)
		{
			m_label.Draw(rc);
			if (m_isFinal)
			{
				m_final.Draw(rc);
				return;
			}
			for (int i = 0; i < m_digitCount; ++i)
			{
				m_digits[i][m_shownDigits[i]].Draw(rc);
			}
		}
	}
}
