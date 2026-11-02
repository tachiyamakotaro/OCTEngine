#pragma once


/**
 * @file MistEffectParameter.h
 * @brief 霧のエフェクト（Mist.efk の煙）の調整値
 */
namespace app
{
	/**
	 * @brief 霧のエフェクトの調整値
	 * @details 今まで DebugScene がばらばらの float で持っていた値を1つにまとめた。
	 *          使うのは DebugScene（煙を出す）、変えるのは InGameEditor（デバッグウィンドウの Display → Mist Effect）。
	 */
	struct MistEffectParameter
	{
		/** 霧のエフェクトを出すか */
		bool isEnabled = true;
		/** 煙1つの大きさ（Mist.efk は大きさ1だと幅10ほどの煙なので、部屋に合わせて大きくする） */
		float puffScale = 40.0f;
		/** 煙を出す間隔（秒。吹き出し口ごとにこの間隔で出す） */
		float puffInterval = 0.25f;
		/** 煙を出す高さの幅（霧の先端からこの高さまでの間に出す） */
		float puffBand = 250.0f;
		/** 煙を出す奥行きのばらつき（±） */
		float puffDepth = 100.0f;
		/** 霧が通ったあと（先端より上）にも煙を出す割合（0〜1。霧が部屋にたまって見える） */
		float fillRate = 0.6f;
		/** 鏡の足場が見えたときに出す煙の大きさ（霧の煙に対する割合） */
		float dewPuffScaleRate = 0.4f;
	};
}
