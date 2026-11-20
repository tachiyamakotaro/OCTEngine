#pragma once
#include "Source/Stage/Goal/GoalParameter.h"


/**
 * @file ResultParameter.h
 * @brief 最後のステージをクリアしたあとの演出（宝物の間）の調整値
 */
namespace app
{
	namespace result
	{
		/**
		 * @brief 宝箱のモデルの形の値（Tools/gen_treasure_models.py と同じ値。モデルを作り直して形を変えたら、ここも合わせる）
		 */
		namespace treasure_shape
		{
			/** 本体の高さ（ふたのちょうつがいの高さ） */
			constexpr float CHEST_HEIGHT = 95.0f;
			/** ふたのちょうつがいの奥行き（本体の奥の辺。本体の真ん中から） */
			constexpr float LID_HINGE_Z = 55.0f;
			/** 宝石が箱の中にあるときの高さ（本体の底から） */
			constexpr float GEM_START_Y = 60.0f;
		}


		/**
		 * @brief 演出の時間（シーンが始まってからの秒）
		 * @details 丸が開く → 走ってくる → 止まる → ふたが開く → 宝石が浮かぶ → 2回跳ねて喜ぶ → GAME CLEAR → ボタンの案内
		 */
		struct CelebrationTiming
		{
			/** 丸が開き始める時間・開ききるまでの時間 */
			float irisOpenStart = 0.2f;
			float irisOpenTime = 1.0f;
			/** 走り始める時間・止まる時間 */
			float runStart = 0.5f;
			float runEnd = 2.4f;
			/** ふたが開き始める時間・開ききるまでの時間 */
			float lidOpenStart = 2.8f;
			float lidOpenTime = 0.8f;
			/** 宝石が浮かび始める時間・浮かびきるまでの時間 */
			float gemRiseStart = 3.2f;
			float gemRiseTime = 1.3f;
			/** 跳ね始める時間・1回の跳ねる時間・次に跳ねるまでの間・回数 */
			float hopStart = 3.9f;
			float hopTime = 0.5f;
			float hopInterval = 0.62f;
			int hopCount = 2;
			/** GAME CLEAR が出始める時間・出きるまでの時間 */
			float logoStart = 4.3f;
			float logoTime = 0.6f;
			/** ボタンの案内が出る時間（ここからタイトルへ戻れる） */
			float promptStart = 5.6f;
			/** カメラが寄りきるまでの時間 */
			float dollyTime = 6.0f;

			/** 跳ね終わる時間 */
			float GetHopEnd() const { return hopStart + hopInterval * static_cast<float>(hopCount - 1) + hopTime; }
		};


		/**
		 * @brief 宝物の間の置き方（ワールドの位置。床の上が y = 0）
		 */
		struct TreasureRoomLayout
		{
			// ---- プレイヤー ----
			/** 走り始める X（画面の左の外）と、止まる X（宝箱の前） */
			float playerStartX = -500.0f;
			float playerStopX = 420.0f;
			/** 跳ねる高さ */
			float hopHeight = 70.0f;
			/** 喜ぶときに、カメラのほうへ振り向く角度（右向き 90 度から足す） */
			float celebrateTurnDeg = 55.0f;

			// ---- 宝箱（台の上） ----
			float chestX = 740.0f;
			float chestY = 80.0f;
			float chestZ = -10.0f;
			/** 金貨の山（宝箱の左右） */
			float coinOffsetX = 165.0f;

			// ---- カメラ（平行投影。ゲームと同じく、注視点の手前から少し上から見る） ----
			float cameraTargetX = 600.0f;
			float cameraTargetY = 230.0f;
			/** 縦に映る範囲（始め → 終わり。だんだん寄る。プレイヤーが画面の 1/4 くらいの高さに見える） */
			float viewHeightStart = 600.0f;
			float viewHeightEnd = 520.0f;
			/** 注視点からカメラまでの距離と高さ（GameCamera と同じ） */
			float cameraDistance = 1100.0f;
			float cameraHeight = 150.0f;
		};


		/**
		 * @brief 宝箱・宝石・光の見た目
		 */
		struct TreasureStyle
		{
			/** ふたが開ききったときの角度（度） */
			float lidOpenAngleDeg = 105.0f;
			/** 宝石が浮かぶ高さ（箱の中から） */
			float gemRiseHeight = 200.0f;
			/** 宝石が回る速さ（度 / 秒）・上下にゆれる幅と速さ */
			float gemSpinSpeedDeg = 50.0f;
			float gemBobHeight = 8.0f;
			float gemBobSpeed = 2.0f;
			/** 宝石の色 */
			stage::GlowColor gemColor = { { 0.55f, 0.9f, 1.0f }, 2.2f };
			/** まわりの光の筋と丸い光の色 */
			stage::GlowColor rayColor = { { 1.0f, 0.86f, 0.5f }, 2.4f };
			/** 光の筋の長さ・丸い光の大きさ・光の筋が回る速さ（度 / 秒） */
			float rayLength = 360.0f;
			float haloSize = 300.0f;
			float raySpinSpeedDeg = 10.0f;
			/** 宝石の光がまわり（床・宝箱・プレイヤー）を照らす距離と強さ（シーンのポイントライトを1つ使う） */
			float lightRange = 1000.0f;
			float lightIntensity = 1.6f;
			float lightColor[3] = { 1.0f, 0.85f, 0.55f };
		};
	}
}
