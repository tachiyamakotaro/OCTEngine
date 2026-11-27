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
			// 端の次は反対の端（回り込む）
			const int next = ((m_selected + step) % m_stageCount + m_stageCount) % m_stageCount;
			if (next == m_selected)
			{
				return false;
			}
			m_selected = next;
			return true;
		}


		bool StageSelectCursor::Jump(const int direction)
		{
			// 札の列が輪になっているので、端を越えたら反対側へ回り込む（左右の Move と同じ）
			return Move(direction >= 0 ? JUMP_STAGE_COUNT : -JUMP_STAGE_COUNT);
		}
	}
}
