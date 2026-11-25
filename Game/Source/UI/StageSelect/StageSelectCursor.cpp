/**
 * @file StageSelectCursor.cpp
 * @brief ステージセレクトのカーソル
 */
#include "stdafx.h"
#include "StageSelectCursor.h"


namespace app
{
	namespace ui
	{
		StageSelectCursor::StageSelectCursor(const int stageCount)
			// ステージが 0 個でも割り算などで壊れないように、1 個はあることにする
			: m_stageCount(stageCount < 1 ? 1 : stageCount)
			, m_selected(0)
		{}


		void StageSelectCursor::Select(const int stageIndex)
		{
			m_selected = stageIndex < 0 ? 0 : (stageIndex >= m_stageCount ? m_stageCount - 1 : stageIndex);
		}


		bool StageSelectCursor::Move(const int step)
		{
			// 全体を1列とみなして動かす（ページはステージの番号から決まるので、端を越えると自然にとなりのページになる）
			const int next = ((m_selected + step) % m_stageCount + m_stageCount) % m_stageCount;
			if (next == m_selected)
			{
				return false;
			}
			m_selected = next;
			return true;
		}


		bool StageSelectCursor::TurnPage(const int step)
		{
			const int pageCount = GetPageCount();
			if (pageCount <= 1)
			{
				return false;
			}
			const int slot = m_selected % STAGES_PER_PAGE;
			const int nextPage = ((GetPage() + step) % pageCount + pageCount) % pageCount;
			// めくった先にその位置がなければ（最後のページが短いとき）、そのページの一番右
			Select(nextPage * STAGES_PER_PAGE + slot);
			return true;
		}


		int StageSelectCursor::GetPageCount() const
		{
			// 切り上げ（例：10 ステージ → 2ページ、11 ステージ → 3ページ）
			return (m_stageCount + STAGES_PER_PAGE - 1) / STAGES_PER_PAGE;
		}


		StagePageRange StageSelectCursor::GetPageRange() const
		{
			StagePageRange range;
			range.first = GetPage() * STAGES_PER_PAGE;
			const int rest = m_stageCount - range.first;
			range.count = rest < STAGES_PER_PAGE ? rest : STAGES_PER_PAGE;
			return range;
		}
	}
}
