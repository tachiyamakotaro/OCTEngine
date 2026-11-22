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
			/**
			 * 寄っている間（寄る〜寄ったまま）はプレイヤーを動けなくするか（時間制限も止まる）
			 * 引き始めたら動けるようになる
			 */
			bool isLockPlayer = false;
		};


		/**
		 * @brief イベントカメラの調整値
		 * @details 値を足すときは、このファイル・EventCameraEditor・仕様書の「調整値」の表の3か所を直す
		 */
		struct EventCameraParameter
		{
			// 並び：isEnabled, zoom, holdTime, margin, isIncludeTarget, blendInTime, blendOutTime, isLockPlayer
			// （ステージ開始・ゴール・死んだときは、それぞれの処理で操作を止めているので isLockPlayer は false）

			/**
			 * ギミックを解いたとき（ステージエディタで Camera Focus にした扉が初めて開いた・溶かす氷に通れる穴が開いた）
			 * 寄っている間はプレイヤーが動けない（ギミックが動く様子を見せる）。引き始めたら動ける
			 */
			EventShotSetting gimmickSolved = { true, 1.0f, 1.5f, 100.0f, true, 0.5f, 0.7f, true };
			/** ゴールに触れたとき（クリアの演出が終わるまで寄ったまま） */
			EventShotSetting goal = { true, 0.8f, 10.0f, 80.0f, true, 0.5f, 0.7f };
			/**
			 * ステージが始まったとき（最初から寄った見え方で、丸が開き始めたら引いていく）
			 * ・寄ったままの時間は StartSequence が決める（holdTime は ImGui の Test のときだけ使う）
			 * ・blendOutTime を StartTiming::irisOpenTime と同じにすると、丸が開ききるのと同時に引ききる
			 */
			EventShotSetting stageStart = { true, 0.7f, 0.6f, 80.0f, false, 0.0f, 1.2f };
			/**
			 * プレイヤーが死んだとき（リスタートまで寄ったまま。リスタートでふだんの見え方に戻る）
			 * ・holdTime は ImGui の Test のときだけ使う
			 */
			EventShotSetting death = { true, 0.8f, 1.5f, 80.0f, false, 0.4f, 0.5f };
		};
	}
}
