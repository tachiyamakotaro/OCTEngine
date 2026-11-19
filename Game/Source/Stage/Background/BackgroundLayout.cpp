/**
 * @file BackgroundLayout.cpp
 * @brief 背景の回廊の区画と松明を並べる
 */
#include "stdafx.h"
#include "BackgroundLayout.h"
#include "Source/Core/Math/MathUtil.h"
#include <cmath>


namespace
{
	/** 小さすぎる大きさで区画が何万個にもならないようにする下限 */
	constexpr float MIN_SCALE = 0.1f;
	/** 円周（ゆらめきの角度） */
	constexpr float TWO_PI = 6.28318530718f;

	/**
	 * @brief マス目の位置から作る乱数（同じ位置・同じ種なら、いつも同じ値）
	 */
	std::uint32_t HashCell(const int column, const int row, const std::uint32_t seed)
	{
		std::uint32_t h = seed * 0x9E3779B9u;
		h ^= static_cast<std::uint32_t>(column) * 0x85EBCA6Bu;
		h ^= static_cast<std::uint32_t>(row) * 0xC2B2AE35u;
		h ^= h >> 16;
		h *= 0x7FEB352Du;
		h ^= h >> 15;
		h *= 0x846CA68Bu;
		h ^= h >> 16;
		return h;
	}

	/** 乱数 → 0 以上 1 未満 */
	float ToUnit(const std::uint32_t value)
	{
		return static_cast<float>(value & 0xFFFFFFu) / static_cast<float>(0x1000000u);
	}

	/** 負の数でも 0〜divisor-1 になる余り */
	int PositiveMod(const int value, const int divisor)
	{
		const int mod = value % divisor;
		return mod < 0 ? mod + divisor : mod;
	}
}


namespace app
{
	namespace stage
	{
		BackgroundLayerLayout BackgroundLayerLayout::Build(const math::Rect& stageBounds, const BackgroundLayoutSetting& setting)
		{
			BackgroundLayerLayout layout;
			const float scale = math::Max(setting.scale, MIN_SCALE);
			const float bayWidth = background_shape::BAY_WIDTH * scale;
			const float tierHeight = background_shape::TIER_HEIGHT * scale;

			// カメラについてくる層は、カメラがステージの端にいるとき、層の「ステージの範囲 × ついてくる割合」だけ外側が見える。
			// それに区画1つぶんの余白を足した範囲を埋める
			const float parallax = math::Clamp01(setting.parallax);
			const math::Vec2 halfSize(stageBounds.Width() * 0.5f, stageBounds.Height() * 0.5f);
			const math::Vec2 expand(halfSize.x * parallax + bayWidth, halfSize.y * parallax + tierHeight);
			layout.m_area = { stageBounds.min - expand, stageBounds.max + expand };

			// マス目はステージの範囲の左下を基準にする（柱がステージの左端にそろう）
			const int firstColumn = static_cast<int>(std::floor((layout.m_area.min.x - stageBounds.min.x) / bayWidth));
			// 右端の柱も置くため、もう1列多く並べる
			const int lastColumn = static_cast<int>(std::ceil((layout.m_area.max.x - stageBounds.min.x) / bayWidth));
			const int firstRow = static_cast<int>(std::floor((layout.m_area.min.y - stageBounds.min.y) / tierHeight));
			const int lastRow = static_cast<int>(std::ceil((layout.m_area.max.y - stageBounds.min.y) / tierHeight)) - 1;

			const float brokenRate = math::Clamp01(setting.brokenRate);
			for (int row = firstRow; row <= lastRow; ++row)
			{
				for (int column = firstColumn; column <= lastColumn; ++column)
				{
					const math::Vec2 foot(stageBounds.min.x + bayWidth * static_cast<float>(column),
						stageBounds.min.y + tierHeight * static_cast<float>(row));
					BackgroundBay bay;
					bay.foot = foot;
					bay.kind = ToUnit(HashCell(column, row, setting.seed)) < brokenRate ? BackgroundBayKind::Broken : BackgroundBayKind::Whole;
					layout.m_bays.push_back(bay);

					if (setting.torchInterval > 0 && PositiveMod(column, setting.torchInterval) == 0)
					{
						BackgroundTorch torch;
						torch.root = math::Vec2(foot.x, foot.y + setting.torchHeight * scale);
						torch.phase = ToUnit(HashCell(column, row, setting.seed + 1u)) * TWO_PI;
						layout.m_torches.push_back(torch);
					}
				}
			}
			return layout;
		}
	}
}
