#pragma once
#include "Source/Core/Math/Rect.h"


/**
 * @file CameraShot.h
 * @brief 演出でカメラに映してほしい見え方（EventCamera が作り、GameCamera が使う）
 */
namespace app
{
	namespace camera
	{
		/**
		 * @brief 演出でカメラに映してほしい見え方
		 * @details ・EventCamera が毎フレーム作って GameCamera に渡す。GameCamera はこれを「ふだんの見え方」に混ぜる。
		 *          ・GameCamera は「なぜ寄るのか（ギミック・ゴール）」を知らなくてよい。映す範囲と寄り具合だけを受け取る。
		 */
		struct CameraShot
		{
			/** 画面に入れたい範囲（余白込み。範囲の真ん中を見る） */
			math::Rect area;
			/**
			 * 必ず画面に入れる範囲（プレイヤーの体 + 余白）
			 * area が遠くまで広がっていて入りきらないときも、ここだけは画面から出さない
			 */
			math::Rect keepArea;
			/** いちばん寄ったときのズーム（範囲が入りきらないときは、入るところまで引く。ふだんより引くことはない） */
			float zoom = 1.0f;
			/** 効き具合（0 = ふだんの見え方 〜 1 = この見え方） */
			float weight = 0.0f;

			/** 演出中か（少しでも効いているか） */
			bool IsActive() const { return weight > 0.0f; }
		};
	}
}
