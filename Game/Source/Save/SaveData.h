/**
 * @file SaveData.h
 * @brief 続きから遊ぶための記録（どのステージから続けるか）
 * @details ファイルは Save/progress.json（実行したフォルダの下。フォルダがなければ作る）。
 *          ・ステージを始めたとき・次のステージへ進むときに SaveProgress（そのステージから続けられる）
 *          ・最後のステージをクリアしたとき・タイトルで「START（始めから）」を選んだときに EraseProgress
 *          ・タイトルは HasProgress で「CONTINUE」を押せるか決め、LoadProgress で続けるステージを知る
 *          ステージは名前と番号の両方を書く。読むときは名前で探す（表の順番を入れ替えても、同じステージから続けられる）。
 *          プレイログの続き（前のログに書き足す）は、PlayLog が自分で覚えている（PlayLog::ContinuePlay）。
 */
#pragma once
#include <optional>


namespace app
{
	namespace save
	{
		/** 続きから遊べるか（記録があって、ステージが表にあるか） */
		bool HasProgress();
		/**
		 * @brief 続きから遊ぶステージ（StageCatalog の表の番号）
		 * @return 記録がない・読めないときは空
		 */
		std::optional<int> LoadProgress();
		/**
		 * @brief このステージから続けられるように書き出す
		 * @param stageIndex StageCatalog の表の番号
		 */
		void SaveProgress(int stageIndex);
		/** 記録を消す（CONTINUE を押せなくなる） */
		void EraseProgress();
	}
}
