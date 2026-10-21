/**
 * @file Rope.cpp
 * @brief ロープ
 */
#include "stdafx.h"
#include "Rope.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		Rope::Rope(const char* name, const float x, const float y, const float width, const float height)
			: BurnableGimmick(name, x, y, width, height)
		{
			igniteTime = 0.8f;
			burnTime = 0.8f;
			spreadTime = 0.15f;
		}


		unsigned int Rope::GetEditorColor() const
		{
			return IM_COL32(200, 170, 110, 255);
		}
	}
}
