#pragma once


/**
 * @file StartParameter.h
 * @brief ステージ開始の演出（丸が開いていく）の調整値
 */
namespace app
{
	namespace stage
	{
		/**
		 * @brief ステージ開始の演出の時間と、最初の丸の大きさ
		 * @details 時間の流れ
		 *            画面の切り替え（SceneManager のフェード）が終わる ─ 小さい丸のまま待つ(holdTime) ─ 丸が開く(irisOpenTime) ─ 終わり
		 *          カメラが引いていく速さは、EventCameraParameter の stageStart.blendOutTime。
		 *          irisOpenTime と同じにすると、丸が開ききるのと同時に引ききる
		 */
		struct StartTiming
		{
			/** 最初の丸の閉じ具合（0 = 開いている 〜 1 = 真っ暗。大きいほど丸が小さい） */
			float startCloseRate = 0.85f;
			/** 画面が見えてから、丸が開き始めるまでの時間（秒。この間は操作できない） */
			float holdTime = 0.6f;
			/** 丸が開ききるまでの時間（秒） */
			float irisOpenTime = 1.2f;
		};
	}
}
