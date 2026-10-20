/**
 * @file RectGimmick.cpp
 * @brief 矩形の範囲を持つギミックの共通部分
 */
#include "stdafx.h"
#include "RectGimmick.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		RectGimmick::RectGimmick(const char* name, const float x, const float y, const float width, const float height)
			: GimmickBase(name)
			, rect(math::Rect::FromMinSize(x, y, width, height))
		{}


		void RectGimmick::SetAnchor(const math::Vec2& anchor)
		{
			// 大きさを保ったまま左下を動かす
			const math::Vec2 size(rect.Width(), rect.Height());
			rect.min = anchor;
			rect.max = anchor + size;
		}


		void RectGimmick::DrawInspector(GimmickInspector& inspector)
		{
			inspector.EditRect(rect);
		}


		void RectGimmick::Serialize(GimmickArchive& archive)
		{
			archive.Rect(rect);
		}


		void RectGimmick::Rotate90(const int quarterTurns)
		{
			// 軸に平行な矩形は、90度回すと幅と高さが入れ替わるだけ（180度なら同じ形）
			if (quarterTurns % 2 == 0)
			{
				return;
			}
			const math::Vec2 center = (rect.min + rect.max) * 0.5f;
			rect = math::Rect::FromCenter(center, math::Vec2(rect.Height(), rect.Width()));
		}
	}
}
