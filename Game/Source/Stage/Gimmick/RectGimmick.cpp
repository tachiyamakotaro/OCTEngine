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
	}
}
