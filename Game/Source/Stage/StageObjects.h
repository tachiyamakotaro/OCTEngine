/**
 * @file StageObjects.h
 * @brief ステージに置く物（奥の鏡と、全ギミック）
 * @details ギミックは1クラス1ファイルで Gimmick フォルダにある。ここではまとめて include するだけ。
 *          新しいギミックを作ったら、ここにも include を1行足す（描画やサウンドなど、ほかの場所から使えるように）。
 *          「信号」= 受光器の点灯 または スイッチが押されていること。名前で指定する。
 */
#pragma once
#include "Gimmick/Checkpoint.h"
#include "Gimmick/DarkZone.h"
#include "Gimmick/Door.h"
#include "Gimmick/EmitRay.h"
#include "Gimmick/FloorSwitch.h"
#include "Gimmick/GimmickBase.h"
#include "Gimmick/Goal.h"
#include "Gimmick/HiddenFloor.h"
#include "Gimmick/IceBlock.h"
#include "Gimmick/Indicator.h"
#include "Gimmick/InvisibleFloor.h"
#include "Gimmick/LightMirror.h"
#include "Gimmick/LightPlatform.h"
#include "Gimmick/LightReceiver.h"
#include "Gimmick/MistEmitter.h"
#include "Gimmick/ShadeBoard.h"
#include "Gimmick/StageBlock.h"
#include "Gimmick/SunLight.h"
#include "Gimmick/WarpZone.h"
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ステージの奥の壁にある大きな鏡
		 * @details 手前の部屋とプレイヤーを映す（見た目だけで、当たり判定や光には関係しない）。
		 *          鏡の面は Z = planeZ の平面。手前(-Z)にあるものが、奥(+Z)に反転して映る。
		 *          frame は鏡の枠（XY平面での範囲）。デバッグ表示とエディタで使う。
		 *          ステージに1つだけなのでギミックにはせず、Stage が直接持つ。
		 */
		struct BackMirror
		{
			/** 鏡を使うか */
			bool isEnabled = false;
			/** 鏡の面のZ座標（床の奥行きの奥端に合わせる） */
			float planeZ = 150.0f;
			/** 鏡の枠（左下とサイズ） */
			math::Rect frame = math::Rect::FromMinSize(0.0f, 0.0f, 1000.0f, 600.0f);


			/** 奥行き z の点が鏡に映ったときの奥行き */
			float ReflectZ(const float z) const { return 2.0f * planeZ - z; }
		};
	}
}
