/**
 * @file PlayLogFile.h
 * @brief プレイログを書き出すファイル（どこに・どう書くか だけ）
 * @details 「何を記録するか」は PlayLog / StageRecord、「どのファイルにどう書くか」はこのクラス、と分けている。
 *          このクラスはプレイログの中身を知らない（文字列を受け取って書くだけ）ので、
 *          書き出す場所や形式を変えたいときは、ここだけ直せばよい。
 *
 *          ファイルは PlayLog/PlayLog_年月日_時分秒.txt（実行したフォルダの下。フォルダがなければ作る）。
 *          同じ名前のファイルがもうあるときは _2、_3 … を付けて、前のファイルを消さないようにする。
 *          メモ帳や Excel で文字化けしないように、UTF-8（BOM 付き）で書く。
 */
#pragma once
#include <ctime>
#include <string>


namespace app
{
	namespace playlog
	{
		/**
		 * @brief プレイログを書き出すファイル
		 */
		class PlayLogFile
		{
		public:
			/**
			 * @brief 新しいファイルを作って書き出す（今までのファイルは消さない）
			 * @param startTime 遊び始めた日時（ファイルの名前に使う）
			 * @param text 書く中身
			 * @return 書けたら true
			 */
			bool SaveAsNewFile(const std::tm& startTime, const std::string& text) const;


		private:
			/** まだないファイルの場所を決める（同じ名前があれば _2、_3 … を付ける） */
			std::string MakeNewPath(const std::tm& startTime) const;
		};
	}
}
