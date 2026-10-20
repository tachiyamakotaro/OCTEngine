#pragma once


/**
 * @file CameraParameter.h
 * @brief ゲームカメラの調整値（ImGui や、将来はステージの JSON から変えられる値をまとめる）
 */
namespace app
{
	namespace camera
	{
		/**
		 * @brief ゲームカメラの調整値
		 * @details 値を足すときは、このファイル・GameCamera::DrawSettingsUI・仕様書の「調整値」の表の3か所を直す
		 */
		struct CameraParameter
		{
			// ---- 追従 ----
			/** 注視点の高さ（見たい位置＝足元からのずらし。体の中ほどを見る） */
			float targetOffsetY = 60.0f;
			/**
			 * 注視点からカメラまでの距離（手前 -Z 方向）
			 * 平行投影では見える範囲は変わらない（変わるのは、奥行き方向にどこまで描画されるかだけ）
			 */
			float followDistance = 1100.0f;
			/** 注視点から見たカメラの高さ（少し上から見下ろすと床の上面が見える） */
			float height = 150.0f;
			/** 追従の速さ（大きいほどすぐ追いつく） */
			float smoothing = 6.0f;

			// ---- 投影 ----
			/** 平行投影にするか（false なら透視投影） */
			bool isOrtho = true;
			/** 平行投影のズーム倍率（1.0 で縦が baseViewHeight。大きいほど引いた視点になる） */
			float zoom = 2.0f;
			/** ズーム 1.0 のときに画面の縦に映る範囲（ワールドの長さ） */
			float baseViewHeight = 720.0f;
			/** 透視投影の画角（度） */
			float viewAngleDeg = 60.0f;
			/** 近平面までの距離 */
			float nearClip = 10.0f;
			/** 遠平面までの距離（奥の鏡に映った世界まで入るように、カメラの距離より十分大きくする） */
			float farClip = 20000.0f;
		};
	}
}
