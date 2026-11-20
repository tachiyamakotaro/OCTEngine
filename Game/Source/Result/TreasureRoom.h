#pragma once


/**
 * @file TreasureRoom.h
 * @brief 宝物の間（最後のステージをクリアしたあとの部屋）の地形を作る
 */
namespace app
{
	namespace stage
	{
		class Stage;
	}


	namespace result
	{
		/**
		 * @brief 宝物の間の地形を作る（床・壁・天井・宝箱の台）
		 * @details ・ステージと同じ地形ブロック（StageBlock）で作るので、StageRenderer（石の見た目）と
		 *            BackgroundRenderer（奥の回廊。地形を囲む矩形に合わせて並ぶ）がそのまま使える
		 *          ・JSON は使わず、C++ で並べる（Stages/Stage01.cpp と同じ書き方）
		 *          ・宝箱の台の高さは TreasureRoomLayout::chestY と合わせる
		 */
		void BuildTreasureRoom(stage::Stage& room);
	}
}
