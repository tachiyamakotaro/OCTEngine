#pragma once
#include "Source/Stage/Goal/GoalParameter.h"


/**
 * @file BackgroundParameter.h
 * @brief ステージの奥の背景（遺跡の回廊）の見た目の調整値
 * @details 値を足すときは、このファイル・BackgroundEditor・仕様書の「調整値」の表の3か所を直す
 */
namespace app
{
	namespace stage
	{
		/**
		 * @brief 背景の層1つぶんの見た目（手前の回廊・奥の回廊・いちばん奥の壁）
		 */
		struct BackgroundLayerStyle
		{
			/** 描くか */
			bool isVisible = true;
			/** 手前の面の Z（地形の奥の面は 150。大きいほど奥） */
			float z = 450.0f;
			/**
			 * カメラについてくる割合（0 = 地形と同じに動く、1 = 画面に貼りついて動かない）
			 * 平行投影だと奥に置いても動き方が変わらないので、これで「遠くの物はゆっくり動く」ように見せる
			 */
			float parallax = 0.15f;
			/** モデルの大きさ（奥の層ほど小さくすると遠くに見える。いちばん奥の壁は使わない） */
			float scale = 1.0f;
			/** 石の色にかける色 */
			float tint[3] = { 1.0f, 1.0f, 1.0f };
			/** まわりの明るさ（シーンの環境光にかける） */
			float ambient = 0.35f;
			/** 平行光の明るさ（シーンの平行光にかける） */
			float sunLight = 0.15f;
			/** 霧の濃さ（0 = 霧なし、1 = 霧の色だけ）。奥の層ほど濃くすると遠くに見える */
			float fog = 0.25f;
		};


		/**
		 * @brief 柱に付ける松明の見た目と、まわりを照らす光
		 */
		struct BackgroundTorchStyle
		{
			/** 出すか */
			bool isVisible = true;
			/** 何本おきの柱に付けるか（0 なら付けない）。変えると並べ直す */
			int pillarInterval = 2;
			/** 区画の床からの炎の根元の高さ（層の大きさをかける前の値）。変えると並べ直す */
			float height = 300.0f;
			/** 光の色 */
			float lightColor[3] = { 1.0f, 0.55f, 0.22f };
			/** 光の強さ */
			float lightIntensity = 1.6f;
			/** 光が届く距離（層の大きさをかける前の値） */
			float lightRange = 650.0f;
			/** ゆらめきの速さ */
			float flickerSpeed = 9.0f;
			/** ゆらめきの大きさ（炎の大きさと光の強さが、この割合だけ変わる） */
			float flickerAmount = 0.12f;
			/** 炎のまわりの色 */
			GlowColor flameOuter = { { 1.0f, 0.42f, 0.08f }, 2.0f };
			/** 炎の真ん中の色 */
			GlowColor flameInner = { { 1.0f, 0.88f, 0.5f }, 3.0f };
			/** 炎の幅（層の大きさをかける前の値） */
			float flameWidth = 13.0f;
			/** 炎の高さ（層の大きさをかける前の値） */
			float flameHeight = 32.0f;
		};


		/**
		 * @brief 背景全体の見た目
		 */
		struct BackgroundStyle
		{
			/** 背景を描くか（false のときは、ステージの奥も外も全部 outsideColor で塗る） */
			bool isVisible = true;

			// ---- 層（手前から順） ----
			/** 手前の回廊（地形のすぐ奥。松明はこの柱に付く） */
			BackgroundLayerStyle nearLayer = { true, 450.0f, 0.15f, 1.0f, { 0.72f, 0.75f, 0.88f }, 0.32f, 0.10f, 0.25f };
			/** 奥の回廊（小さく・暗く・ゆっくり動かして、遠くに見せる） */
			BackgroundLayerStyle farLayer = { true, 1000.0f, 0.40f, 0.55f, { 0.65f, 0.7f, 0.85f }, 0.26f, 0.06f, 0.60f };
			/** いちばん奥の壁（ステージの範囲を全部ふさぐ。回廊のすき間から見える） */
			BackgroundLayerStyle wall = { true, 1500.0f, 0.60f, 1.0f, { 0.6f, 0.65f, 0.8f }, 0.25f, 0.04f, 0.80f };

			// ---- 松明 ----
			/** 手前の回廊の松明 */
			BackgroundTorchStyle nearTorch;
			/** 奥の回廊の松明（遠くの小さな明かり） */
			BackgroundTorchStyle farTorch = { true, 3, 300.0f, { 1.0f, 0.5f, 0.2f }, 1.3f, 650.0f, 9.0f, 0.12f,
				{ { 1.0f, 0.42f, 0.08f }, 2.0f }, { { 1.0f, 0.88f, 0.5f }, 3.0f }, 13.0f, 32.0f };

			// ---- 並べ方（変えると並べ直す） ----
			/** アーチが崩れた区画の割合（0〜1） */
			float brokenRate = 0.2f;
			/** 崩れた区画を選ぶ乱数の種（変えると崩れる場所が変わる） */
			int seed = 1;

			// ---- 色 ----
			/** 霧の色（奥の層ほどこの色に近づく。少し明るい青にすると、遠くがかすんで見える） */
			float fogColor[3] = { 0.07f, 0.085f, 0.13f };
			/** ステージの外（地形を囲む矩形の外）の色 */
			float outsideColor[3] = { 0.0f, 0.0f, 0.0f };
			/** 石のテクスチャ1枚ぶんのワールドの長さ（地形の stoneTextureWorldSize と同じにすると石の大きさがそろう） */
			float stoneTextureWorldSize = 200.0f;
		};
	}
}
