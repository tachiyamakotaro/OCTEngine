/**
 * @file StageCatalog.cpp
 * @brief ステージの一覧
 */
#include "stdafx.h"
#include "StageCatalog.h"
#include "Source/Timer/TimerParameter.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/** 今遊ぶステージの番号 */
			int s_currentStageIndex = FIRST_STAGE_INDEX;
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


		float GetCurrentStageTimeLimit()
		{
			const float limit = GetCurrentStageInfo().timeLimitSeconds;
			// 0（書いていない）なら初期値。マイナスはそのまま返す（時間制限なし）
			return limit == 0.0f ? timer::DEFAULT_TIME_LIMIT : limit;
		}
	}
}
