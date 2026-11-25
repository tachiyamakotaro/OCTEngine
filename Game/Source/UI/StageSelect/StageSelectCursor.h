/**
 * @file StageSelectCursor.h
 * @brief ステージセレクトの、どのステージを選んでいるか・今どのページか（計算だけ。画像は持たない）
 * @details 1ページに STAGES_PER_PAGE（5）ステージずつ並べる。
 *            1ページ目 … STAGE 1〜5
 *            2ページ目 … STAGE 6〜10
 *            3ページ目 … STAGE 11〜（ステージが 11 個以上になったら自動で増える）
 *          ページの数はステージの数（StageCatalog の STAGE_COUNT）から決めるので、
 *          ステージを足したり減らしたりしても、ここは直さなくてよい。
 *          最後のページは 5 個より少ないことがある（例：11 ステージなら 3ページ目は 1 個）。
 */
#pragma once


namespace app
{
	namespace ui
	{
		/**
		 * @brief 1ページに並ぶステージの範囲（StageCatalog の表の番号で [first, first + count)）
		 */
		struct StagePageRange
		{
			/** ページの最初のステージ */
			int first = 0;
			/** ページに並ぶステージの数 */
			int count = 0;

			/** そのステージがこのページにあるか */
			bool Contains(const int stageIndex) const
			{
				return stageIndex >= first && stageIndex < first + count;
			}
			/** そのステージが、ページの左から何番目か（0 から） */
			int SlotOf(const int stageIndex) const
			{
				return stageIndex - first;
			}
		};


		/**
		 * @brief ステージセレクトのカーソル（選んでいるステージとページ）
		 */
		class StageSelectCursor
		{
		public:
			/** 1ページに並べるステージの数 */
			static constexpr int STAGES_PER_PAGE = 5;


		public:
			/**
			 * @param stageCount ステージの数（StageCatalog の STAGE_COUNT）
			 */
			explicit StageSelectCursor(int stageCount);

			/** そのステージを選ぶ（範囲外なら一番近いステージ。ページもそのステージのページになる） */
			void Select(int stageIndex);
			/**
			 * @brief 1つ左・右のステージへ動かす
			 * @details ページの端を越えたら、となりのページへ移る。最後のステージの次は最初のステージ（逆も）
			 * @param step +1 で右、-1 で左
			 * @return 動いたら true（ステージが1つしかないときは false）
			 */
			bool Move(int step);
			/**
			 * @brief ページをめくる
			 * @details ページの中の位置（左から何番目か）はそのまま。めくった先のページにその位置がなければ、ページの一番右のステージ。
			 *          最後のページの次は最初のページ（逆も）
			 * @param step +1 で次のページ、-1 で前のページ
			 * @return めくれたら true（1ページしかないときは false）
			 */
			bool TurnPage(int step);

			/** 選んでいるステージ（StageCatalog の表の番号） */
			int GetSelectedStage() const { return m_selected; }
			/** 今のページ（0 から） */
			int GetPage() const { return m_selected / STAGES_PER_PAGE; }
			/** ページの数（ステージの数から決まる） */
			int GetPageCount() const;
			/** 今のページに並ぶステージの範囲 */
			StagePageRange GetPageRange() const;


		private:
			/** ステージの数 */
			int m_stageCount;
			/** 選んでいるステージ */
			int m_selected;
		};
	}
}
