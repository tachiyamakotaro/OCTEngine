#pragma once
#include "Source/Core/Math/Rect.h"


/**
 * @file CameraStageBounds.h
 * @brief カメラの全体表示で映す「ステージの範囲」を求める
 */
namespace app
{
	namespace stage
	{
		class Stage;
	}


	namespace camera
	{
		/**
		 * @brief ステージの範囲（全部の地形ブロックを囲む矩形）を求める
		 * @details ステージエディタの「Fit View」と同じく、地形ブロック（StageBlock）だけを見る。
		 *          ギミックは地形の中に置くので、地形が入ればギミックも入る。
		 * @param stage ステージ
		 * @param outBounds 求めた範囲（左下 min・右上 max）
		 * @return 地形ブロックが1つもないときは false（outBounds は変えない）
		 */
		bool CalcStageBounds(const stage::Stage& stage, math::Rect& outBounds);
	}
}
