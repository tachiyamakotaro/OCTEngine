/**
 * @file CameraStageBounds.cpp
 * @brief カメラの全体表示で映す「ステージの範囲」を求める
 */
#include "stdafx.h"
#include "CameraStageBounds.h"

#include "Source/Core/Math/MathUtil.h"
#include "Source/Stage/Stage.h"


namespace app
{
	namespace camera
	{
		bool CalcStageBounds(const stage::Stage& stage, math::Rect& outBounds)
		{
			bool isFound = false;
			math::Rect bounds;
			for (const stage::StageBlock& block : stage.GetGimmicks().View<stage::StageBlock>())
			{
				if (!isFound)
				{
					bounds = block.rect;
					isFound = true;
					continue;
				}
				// 今までの範囲と、このブロックの両方が入るように広げる
				bounds.min.x = math::Min(bounds.min.x, block.rect.min.x);
				bounds.min.y = math::Min(bounds.min.y, block.rect.min.y);
				bounds.max.x = math::Max(bounds.max.x, block.rect.max.x);
				bounds.max.y = math::Max(bounds.max.y, block.rect.max.y);
			}
			if (isFound)
			{
				outBounds = bounds;
			}
			return isFound;
		}
	}
}
