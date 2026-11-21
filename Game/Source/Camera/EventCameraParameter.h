#pragma once


/**
 * @file EventCameraParameter.h
 * @brief イベントカメラ（特定の場面でだけ寄る）の調整値
 */
namespace app
{
	namespace camera
	{
		/**
		 * @brief 場面1つぶんの寄り方
		 */
		struct EventShotSetting
		{
			/** この場面で寄るか */
			bool isEnabled = true;
			/**
			 * いちばん寄ったときのズーム（ふだんは CameraParameter::zoom = 2.0。小さいほど寄る）
			 * 一緒に映す範囲が入りきらないときは、入るところまで引く（ふだんより引くことはない）
			 */
			float zoom = 1.3f;
			/** 寄ったままにする時間（秒） */
			float holdTime = 1.5f;
			/** 映す範囲のまわりに空ける余白（ワールドの長さ。上下左右それぞれ） */
			float margin = 120.0f;
			/** ギミック・ゴールも一緒に映すか（false ならプレイヤーだけを映す） */
			bool isIncludeTarget = true;
			/** 寄るまでの時間（秒。0 なら最初から寄った見え方） */
			float blendInTime = 0.5f;
			/** ふだんの見え方に戻るまでの時間（秒） */
			float blendOutTime = 0.7f;
		};


		/**
		 * @brief イベントカメラの調整値
		 * @details 値を足すときは、このファイル・EventCameraEditor・仕様書の「調整値」の表の3か所を直す
		 */
		struct EventCameraParameter
		{
			// 並び：isEnabled, zoom, holdTime, margin, isIncludeTarget, blendInTime, blendOutTime

			/** ギミックを解いたとき（扉が初めて開いた・溶かす氷に通れる穴が開いた） */
			EventShotSetting gimmickSolved = { true, 1.3f, 1.5f, 120.0f, true, 0.5f, 0.7f };
			/** ゴールに触れたとき（クリアの演出が終わるまで寄ったまま） */
			EventShotSetting goal = { true, 1.1f, 10.0f, 150.0f, true, 0.5f, 0.7f };
			/**
			 * ステージが始まったとき（最初から寄った見え方で、丸が開き始めたら引いていく）
			 * ・寄ったままの時間は StartSequence が決める（holdTime は ImGui の Test のときだけ使う）
			 * ・blendOutTime を StartTiming::irisOpenTime と同じにすると、丸が開ききるのと同時に引ききる
			 */
			EventShotSetting stageStart = { true, 1.1f, 0.6f, 120.0f, false, 0.0f, 1.2f };
		};
	}
}
