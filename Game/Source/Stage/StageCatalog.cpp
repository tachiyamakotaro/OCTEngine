/**
 * @file StageCatalog.cpp
 * @brief ステージの一覧
 */
#include "stdafx.h"
#include "StageCatalog.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/** 今遊ぶステージの番号 */
			int s_currentStageIndex = 0;
		}


		int GetCurrentStageIndex()
		{
			return s_currentStageIndex;
		}


		void SetCurrentStageIndex(const int index)
		{
			s_currentStageIndex = index < 0 ? 0 : (index >= STAGE_COUNT ? STAGE_COUNT - 1 : index);
		}


		const StageInfo& GetCurrentStageInfo()
		{
			return STAGE_INFO_TABLE[s_currentStageIndex];
		}
	}
}
