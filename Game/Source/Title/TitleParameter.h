#pragma once


/**
 * @file TitleParameter.h
 * @brief タイトル画面（遺跡の入口）の調整値
 * @details 遺跡の外の夕暮れ。プレイヤーが左から走ってきて、遺跡の門の前で止まり、門のほうを向くと扉が開いて中の光がもれる
 */
namespace app
{
	namespace title
	{
		/**
		 * @brief 演出の時間（秒）
		 * @details ・始まったとき（シーンが始まってからの秒）
		 *              走ってくる → 門の少し手前で止まって待つ
		 *          ・START / CONTINUE を押したとき（押してからの秒。走ってくる途中で押したら、止まってから）
		 *              門の前まで走る → 門のほうを向く → 扉が開く → 次のシーンへ
		 *          メニューは最初から操作できる（演出を待たなくてよい）。ロゴの出る時間は TitleMenu.cpp の LOGO_APPEAR_～
		 */
		struct EntranceTiming
		{
			// ---- 始まったとき ----
			/** 走り始める時間・門の少し手前で止まる時間（止まる前はゆっくり） */
			float runStart = 0.4f;
			float runEnd = 3.0f;

			// ---- START / CONTINUE を押したとき ----
			/** 門の前まで走る時間 */
			float approachTime = 0.8f;
			/** 門のほうを向ききるまでの時間（門の前に着いてから） */
			float turnTime = 0.4f;
			/** 扉が開き始める時間（押してから） */
			float doorOpenDelay = 1.0f;
			/** 扉が開ききるまでの時間（ゴールの門の ClearSequence に渡す。ゲームの 0.9 秒と同じくらい） */
			float doorOpenTime = 1.0f;
			/** 次のシーンへ移り始める時間（押してから。扉が開ききって、光が少し見えてから） */
			float leaveDelay = 2.3f;
		};


		/**
		 * @brief 置き方（ワールドの位置。地面の上が y = 0）
		 * @details 遺跡の建物の形は RuinEntrance.cpp（門の位置 gateX からの距離で決めている）
		 */
		struct RuinEntranceLayout
		{
			// ---- 門 ----
			/** 門の真ん中の X */
			float gateX = 450.0f;
			/**
			 * 門の大きさ（1 = ステージのゴールの門と同じ。幅 252・高さ 272 が、1.4 で 353・381 になる）
			 * 門は遺跡の壁より手前に置く（奥行きは RuinEntrance.cpp で、壁の手前の面と門の奥行きから決める）
			 */
			float gateScale = 1.4f;

			// ---- プレイヤー ----
			/** 走り始める X（画面の左の外） */
			float playerStartX = -800.0f;
			/** 始まったときに止まって待つ X（門の少し手前。門の左の柱より左） */
			float playerWaitX = 150.0f;
			/** START / CONTINUE を押したあとに止まる X（門の前。gateX とそろえる） */
			float playerStopX = 450.0f;
			/** 走る所の奥行き（門の手前の面 -225 より手前。地面の手前の面は -300） */
			float playerZ = -280.0f;
			/** 門のほうを向いたときの角度（0 = 奥を向いて背中が見える、90 = 右向き。少し横顔が見えるように） */
			float faceGateDeg = 15.0f;

			// ---- カメラ（平行投影。ゲームと同じく、注視点の手前から少し上から見る） ----
			float cameraTargetX = 0.0f;
			float cameraTargetY = 200.0f;
			/** 縦に映る範囲（門を大きくしたので少し広く。プレイヤーは画面の 1/6 くらいの高さに見える） */
			float viewHeight = 760.0f;
			/** 注視点からカメラまでの距離と高さ（GameCamera と同じ） */
			float cameraDistance = 1100.0f;
			float cameraHeight = 150.0f;

			// ---- 空の板 ----
			/** 空の板を置く奥行き（いちばん奥） */
			float skyZ = 4000.0f;
			/** 空の板の大きさ（カメラに映る範囲より十分大きく） */
			float skySize = 8000.0f;
		};


		/**
		 * @brief 扉の奥からもれる光（シーンのポイントライトを1つ使う。扉が開くほど明るくなる）
		 */
		struct GateLightStyle
		{
			/** 門の足元からの高さと奥行き（門の足元からの Z。手前に置くと、門の前に立つプレイヤーと地面が照らされる） */
			float height = 160.0f;
			float z = -60.0f;
			/** 届く距離と強さ */
			float range = 650.0f;
			float intensity = 1.8f;
			float color[3] = { 1.0f, 0.75f, 0.42f };
		};


		/**
		 * @brief 夕暮れの空の見た目（titleSky.fx に渡す）
		 * @details 高さは画面の割合（v = 0 が上、1 が下）。横は画面の高さを 1 とした長さ（16:9 なら 0〜1.78）
		 */
		struct SkyStyle
		{
			/** いちばん上の色（夜に近い紺） */
			float topColor[3] = { 0.04f, 0.06f, 0.18f };
			/** 真ん中の色（紫）と、その色になる高さ */
			float middleColor[3] = { 0.24f, 0.19f, 0.42f };
			float middleV = 0.38f;
			/** 地平線の色（夕焼けのオレンジ）と、地平線の高さ（地面の奥の辺のあたり） */
			float horizonColor[3] = { 0.98f, 0.55f, 0.30f };
			float horizonV = 0.70f;

			/** 太陽の位置（u, v）・大きさ・まわりの光の広がり */
			float sunU = 0.50f;
			float sunV = 0.61f;
			float sunSize = 0.035f;
			float sunGlowSize = 0.22f;
			/** 太陽の色と光る強さ（1 より大きいとブルームで光があふれる） */
			float sunColor[3] = { 1.0f, 0.72f, 0.42f };
			float sunGlow = 1.3f;

			/** 遠くの山の色・ふもとの高さ・山の高さ */
			float farMountainColor[3] = { 0.36f, 0.24f, 0.42f };
			float farMountainBaseV = 0.72f;
			float farMountainHeight = 0.16f;
			/** 近くの丘の色・ふもとの高さ・丘の高さ */
			float nearMountainColor[3] = { 0.16f, 0.12f, 0.26f };
			float nearMountainBaseV = 0.74f;
			float nearMountainHeight = 0.08f;
			/** 山のふもとにかかるもやの濃さ（0〜1。地平線の色に寄せる） */
			float hazeAmount = 0.55f;
			/** 星の明るさ（0 なら出さない） */
			float starBrightness = 0.8f;
		};
	}
}
