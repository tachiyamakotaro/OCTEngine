/**
 * @file GimmickIconTable.h
 * @brief ステージが始まる前の表示に出す、ギミックのアイコンの表
 * @details そのステージに置いてあるギミック（JSON の "type"）を見て、表の上から順に、当てはまるアイコンを並べる。
 *
 *          新しいギミックのアイコンを足すとき
 *            1. 256 x 256 の画像を Assets/sprite/stage_intro/ に置く（Tools/gen_gimmick_icons.py で作った物と同じ形）
 *            2. GIMMICK_ICON_TABLE に1行足す（{ 画像, { アイコンを出すギミックの "type", ... } }）
 *          並べる順番は、この表の順番。1つのアイコンに "type" を MAX_TYPES_PER_ICON 個まで書ける
 *          （どれか1つでも置いてあれば出す）。
 */
#pragma once
#include <array>
#include <iterator>


namespace app
{
	namespace ui
	{
		/** 1つのアイコンに書ける "type" の数 */
		constexpr int MAX_TYPES_PER_ICON = 4;

		/**
		 * @brief アイコン1つぶん
		 */
		struct GimmickIcon
		{
			/** アイコンの画像（256 x 256） */
			const char* imagePath;
			/** このアイコンを出すギミックの種類（JSON の "type"。使わない所は nullptr） */
			std::array<const char*, MAX_TYPES_PER_ICON> types;
		};

		/**
		 * @brief アイコンの表（上から順に並べる）
		 */
		constexpr GimmickIcon GIMMICK_ICON_TABLE[] = {
			// 反射鏡
			{ "Assets/sprite/stage_intro/icon_mirror.dds", { "LightMirror" } },
			// 氷
			{ "Assets/sprite/stage_intro/icon_ice.dds", { "IceBlock" } },
			// 暗闇（暗い場所・照らすと見える足場）
			{ "Assets/sprite/stage_intro/icon_dark.dds", { "DarkZone", "HiddenFloor" } },
			// ソーラーパネル（光をためると動く足場）
			{ "Assets/sprite/stage_intro/icon_solar.dds", { "MovingBlock" } },
			// 色の光（色センサー・色ガラス）
			{ "Assets/sprite/stage_intro/icon_color.dds", { "ColorReceiver", "ColorGlass" } },
		};

		/** アイコンの種類の数 */
		constexpr int GIMMICK_ICON_COUNT = static_cast<int>(std::size(GIMMICK_ICON_TABLE));
	}
}
