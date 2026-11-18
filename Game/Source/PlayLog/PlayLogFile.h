/**
 * @file PlayLogFile.h
 * @brief プレイログを書き出すファイル（どこに・どう書くか だけ）
 * @details 「何を記録するか」は PlayLog / StageRecord、「どのファイルにどう書くか」はこのクラス、と分けている。
 *          このクラスはプレイログの中身を知らない（文字列を受け取って書くだけ）ので、
 *          書き出す場所や形式を変えたいときは、ここだけ直せばよい。
 *
 *          ファイルは PlayLog/PlayLog_年月日_時分秒.txt（実行したフォルダの下。フォルダがなければ作る）。
 *          新しいプレイのときは、同じ名前のファイルがもうあれば _2、_3 … を付けて、前のファイルを消さないようにする。
 *          続きから遊んだプレイは、前と同じファイルに書き直す（前のプレイの記録も入れて書くので、消える物はない）。
 *          続きから遊ぶための記録（まだ途中のプレイの中身）は Save/playlog_continue.json に置く。
 *          メモ帳や Excel で文字化けしないように、UTF-8（BOM 付き）で書く。
 */
#pragma once
#include <ctime>
#include <optional>
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
			/**
			 * @brief 決まったファイルに書き出す（もうあれば書き直す）
			 * @param path MakeNewPath で決めたファイル
			 * @param text 書く中身
			 * @return 書けたら true
			 */
			bool Save(const std::string& path, const std::string& text) const;
			/** まだないファイルの場所を決める（同じ名前があれば _2、_3 … を付ける） */
			std::string MakeNewPath(const std::tm& startTime) const;

			/** 続きから遊ぶための記録を書き出す（JSON の文字列） */
			bool SaveContinueData(const std::string& text) const;
			/** 続きから遊ぶための記録を読む（なければ空） */
			std::optional<std::string> LoadContinueData() const;
			/** 続きから遊ぶための記録を消す */
			void EraseContinueData() const;
		};
	}
}
