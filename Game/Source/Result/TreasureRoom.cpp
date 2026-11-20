/**
 * @file TreasureRoom.cpp
 * @brief 宝物の間の地形を作る
 */
#include "stdafx.h"
#include "TreasureRoom.h"
#include "Source/Stage/Stage.h"


namespace app
{
	namespace result
	{
		void BuildTreasureRoom(stage::Stage& room)
		{
			using stage::StageBlock;

			room.Clear();
			room.SetName("TreasureRoom");

			// 部屋の外枠（カメラに映るのは床の上の真ん中あたりだけ。壁と天井は背景の範囲を決めるために置く）
			room.Add<StageBlock>("Floor", -1600.0f, -400.0f, 3200.0f, 400.0f);
			room.Add<StageBlock>("Ceiling", -1600.0f, 1000.0f, 3200.0f, 400.0f);
			room.Add<StageBlock>("LeftWall", -1600.0f, 0.0f, 300.0f, 1000.0f);
			room.Add<StageBlock>("RightWall", 1300.0f, 0.0f, 300.0f, 1000.0f);

			// 宝箱の台（2段。上の段の上が y = 80 = TreasureRoomLayout::chestY。真ん中が chestX = 740）
			// プレイヤーは台の左（playerStopX = 420）の床に止まる
			room.Add<StageBlock>("DaisLower", 470.0f, 0.0f, 540.0f, 40.0f);
			room.Add<StageBlock>("DaisUpper", 550.0f, 40.0f, 380.0f, 40.0f);
		}
	}
}
