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
		 * @brief 調整値の範囲（GameCamera と CameraEditor の両方で使う）
		 */
		namespace limit
		{
			/** カメラの距離の範囲 */
			constexpr float MIN_DISTANCE = 500.0f;
			constexpr float MAX_DISTANCE = 6000.0f;
			/** 全体表示（透視投影）のときの距離の上限（ステージ全体が入るように、ふだんより遠くまで引ける） */
			constexpr float MAX_OVERVIEW_DISTANCE = 60000.0f;
			/** 平行投影のズームの範囲 */
			constexpr float MIN_ZOOM = 0.25f;
			constexpr float MAX_ZOOM = 5.0f;
			/** 全体表示のときのズームの上限（ステージ全体が入るように、ふだんより大きくできる） */
			constexpr float MAX_OVERVIEW_ZOOM = 40.0f;
			/** 透視投影の画角の範囲（度） */
			constexpr float MIN_VIEW_ANGLE = 10.0f;
			constexpr float MAX_VIEW_ANGLE = 120.0f;
		}


		/**
		 * @brief ゲームカメラの調整値
		 * @details 値を足すときは、このファイル・CameraEditor::Draw・仕様書の「調整値」の表の3か所を直す
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
			/** 追従の速さ（大きいほどすぐ追いつく。全体表示との切り替えの速さもこれ） */
			float smoothing = 6.0f;

			// ---- 全体表示（ステージ全体を真ん中から映す） ----
			/** 全体表示にするか（true の間はプレイヤーを追わず、ステージの真ん中から全体が入るように映す） */
			bool isOverview = false;
			/** 全体表示のとき、ステージのまわりに空ける余白（ワールドの長さ。上下左右それぞれ） */
			float overviewMargin = 200.0f;

			// ---- 投影 ----
			/** 平行投影にするか（false なら透視投影） */
			bool isOrtho = true;
			/** 平行投影のズーム倍率（1.0 で縦が baseViewHeight。大きいほど引いた視点になる。全体表示のときは自動で決める） */
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
