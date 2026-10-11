/**
 * @file Stage01.cpp
 * @brief ステージ1の定義
 * @details 部屋1（x -100〜1500）：光の部屋
 *          1. 床の高さの光（Lamp）が反射鏡 M1 に当たっているが、M1 が縦向きなので光は左へ跳ね返っている
 *          2. M1 の前で右を向いてアクション → M1 が45°傾き、光が真上の受光器 R1 に当たる
 *          3. R1 が点灯して扉 Door1 が開く
 *          4. 扉が開くと、扉に遮られていたもう一本の光（Lamp2）が奥へ届き、光の足場 Step が現れる
 *          5. Step に乗って、普通のジャンプでは届かない高台（HighLedge）の崖をつかんで登る
 *          6. 高台の右の壁の穴（y 450〜650）から部屋2へ
 *
 *          部屋2（x 1600〜2700）：霧の部屋（縦に長い部屋）
 *          1. 左下から足場 Step2_1〜Step2_4 を右上へ乗り継ぐ（1段 150 ずつ高くなる）
 *          2. Step2_4 の上から真上にジャンプして、右上の棚（SwitchLedge）の崖をつかんで登る
 *          3. 棚の上のスイッチ Switch1 を踏む → 天井から霧が吹き出して下へ広がる
 *          4. 霧が触れると、風景を映して見えなかった鏡の足場 MirrorStep1〜3 が見えるようになる
 *             （露が残るので、霧が晴れても見えたまま）
 *          5. 鏡の足場を左上へ乗り継いで、出口の棚（ExitLedge）へ。その上の天井の穴が次の部屋への出口
 *
 *          奥の壁は2部屋とも一面の鏡（BackMirror）で、部屋とプレイヤーが映る
 *
 *          座標の単位は k2EngineLow のワールド単位。(x, y) はブロックなどは左下、鏡は中心。
 *          ジャンプの高さは約200（足場は1段150まで）、崖つかまりなら頭の上40まで届く。
 */
#include "stdafx.h"
#include "Stage01.h"
#include "Source/Stage/Stage.h"


namespace app
{
	namespace stage
	{
		void BuildStage01(Stage& stage)
		{
			stage.Clear();
			stage.SetName("Stage01");
			// 奥の壁一面の鏡（面は床の奥端 z=150。枠は2部屋の内側全体）
			stage.SetBackMirror(true, 150.0f, -100.0f, 0.0f, 2800.0f, 1850.0f);

			// 地形ブロック
			stage.AddBlock("Floor", -200.0f, -100.0f, 1700.0f, 100.0f);
			stage.AddBlock("LeftWall", -200.0f, 0.0f, 100.0f, 800.0f);
			stage.AddBlock("HighLedge", 1100.0f, 0.0f, 400.0f, 450.0f);
			// 部屋1と部屋2の間の壁（高台と同じ高さ y=450 から 200 の穴を空けて、部屋2へ通れるようにする）
			stage.AddBlock("RightWallLower", 1500.0f, -100.0f, 100.0f, 550.0f);
			stage.AddBlock("RightWallUpper", 1500.0f, 650.0f, 100.0f, 1300.0f);
			stage.AddBlock("ReceiverMount", 250.0f, 540.0f, 100.0f, 30.0f);

			// ==== 部屋2（霧の部屋）====
			// 床（上面 y=450 は部屋1の高台と同じ高さ）・右の壁・天井（左端 x 1600〜1850 は出口の穴）
			stage.AddBlock("Floor2", 1600.0f, -100.0f, 1100.0f, 550.0f);
			stage.AddBlock("RightWall2", 2700.0f, -100.0f, 100.0f, 2050.0f);
			stage.AddBlock("Ceiling2", 1850.0f, 1850.0f, 950.0f, 100.0f);
			// 左下から右上へ乗り継ぐ足場（1段目は床から立つ台、上面 600 / 750 / 900 / 1050）
			stage.AddBlock("Step2_1", 1700.0f, 450.0f, 200.0f, 150.0f);
			stage.AddBlock("Step2_2", 1950.0f, 710.0f, 200.0f, 40.0f);
			stage.AddBlock("Step2_3", 2200.0f, 860.0f, 200.0f, 40.0f);
			stage.AddBlock("Step2_4", 2450.0f, 1010.0f, 250.0f, 40.0f);
			// スイッチの棚（上面 1350。Step2_4 から真上にジャンプして崖をつかむ）
			stage.AddBlock("SwitchLedge", 2560.0f, 1310.0f, 140.0f, 40.0f);
			// 出口の棚（上面 1680。この上の天井の穴が次の部屋への出口）
			stage.AddBlock("ExitLedge", 1600.0f, 1640.0f, 120.0f, 40.0f);

			// 光源
			{
				auto& source = stage.AddLightSource("Lamp", -90.0f, 60.0f, 0.0f);
				source.isOn = true;
			}
			{
				auto& source = stage.AddLightSource("Lamp2", -90.0f, 232.0f, 0.0f);
				source.isOn = true;
			}

			// 反射鏡
			{
				auto& mirror = stage.AddLightMirror("M1", 300.0f, 60.0f, 90.0f, 80.0f);
				mirror.isDoubleSided = false;
				mirror.isRotatable = true;
				mirror.rotationStepDeg = 45.0f;
				mirror.isMovable = true;
				mirror.hasRail = true;
				mirror.railMinX = 150.0f;
				mirror.railMaxX = 700.0f;
			}

			// 受光器
			stage.AddLightReceiver("R1", 280.0f, 500.0f, 40.0f, 40.0f);

			// 扉
			{
				auto& door = stage.AddDoor("Door1", 900.0f, 0.0f, 40.0f, 300.0f, { "R1" });
				door.openSpeed = 2.0f;
				door.isLatch = false;
			}

			// 光の足場
			{
				auto& platform = stage.AddLightPlatform("Step", 950.0f, 220.0f, 150.0f, 25.0f);
				platform.lingerTime = 0.15f;
			}

			// チェックポイント（部屋2の入口。部屋2で落ちて死んだらここから再開）
			stage.AddCheckpoint("Room2Entrance", 1600.0f, 450.0f, 80.0f, 200.0f);

			// 床スイッチ（棚の上面に薄く置く。踏んだら押されたまま）
			{
				auto& floorSwitch = stage.AddFloorSwitch("Switch1", 2600.0f, 1350.0f, 70.0f, 8.0f);
				floorSwitch.isLatch = true;
			}

			// 霧の発生装置（部屋2全体。Switch1 で天井から吹き出す）
			{
				auto& mist = stage.AddMistEmitter("Mist1", 1600.0f, 450.0f, 1100.0f, 1400.0f, "Switch1");
				mist.fallSpeed = 500.0f;
				mist.duration = 3.0f;
				mist.fadeTime = 1.5f;
				mist.ventSpacing = 220.0f;
			}

			// 鏡の足場（スイッチの棚から左上へ。下の足場からのジャンプで頭をぶつけないよう、下面を y 1400 より上にしている）
			{
				auto& platform = stage.AddMirrorPlatform("MirrorStep1", 2300.0f, 1400.0f, 150.0f, 25.0f);
				platform.isSolidWhenHidden = true;
			}
			{
				auto& platform = stage.AddMirrorPlatform("MirrorStep2", 2000.0f, 1480.0f, 150.0f, 25.0f);
				platform.isSolidWhenHidden = true;
			}
			{
				auto& platform = stage.AddMirrorPlatform("MirrorStep3", 1750.0f, 1560.0f, 150.0f, 25.0f);
				platform.isSolidWhenHidden = true;
			}
		}
	}
}
