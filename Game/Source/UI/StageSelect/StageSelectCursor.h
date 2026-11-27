/**
 * @file StageSelectCursor.h
 * @brief ステージセレクトで、どのステージを選んでいるか（計算だけ。画像は持たない）
 * @details ページはない。全部のステージを1列に並べて、その中の1つを選ぶ。
 *            ・左右 … 1つずつ（最後のステージの次は最初のステージ。逆も）
 *            ・LB / RB … JUMP_STAGE_COUNT（5）ずつ飛ぶ（端を越えたら反対側へ回り込む。札の列が輪になっているので）
 *          ステージの数は StageCatalog の STAGE_COUNT から受け取るので、ステージを足したり減らしたりしても、ここは直さなくてよい。
 *          札をどこにどの大きさで置くか（横に流れる見た目）は StageCarousel。
 */
#pragma once


namespace app
{
	namespace ui
	{
		/**
		 * @brief ステージセレクトのカーソル（選んでいるステージ）
		 */
		class StageSelectCursor
		{
		public:
			/** LB / RB で1回に飛ぶステージの数 */
			static constexpr int JUMP_STAGE_COUNT = 5;


		public:
			/**
			 * @param stageCount ステージの数（StageCatalog の STAGE_COUNT）
			 */
			explicit StageSelectCursor(int stageCount);

			/** そのステージを選ぶ（範囲外なら一番近いステージ） */
			void Select(int stageIndex);
			/**
			 * @brief 1つ左・右のステージへ動かす（最後のステージの次は最初のステージ。逆も）
			 * @param step +1 で右、-1 で左
			 * @return 動いたら true（ステージが1つしかないときは false）
			 */
			bool Move(int step);
			/**
			 * @brief JUMP_STAGE_COUNT ずつ飛ぶ（端を越えたら反対側へ回り込む）
			 * @param direction +1 で右（先のステージ）、-1 で左
			 * @return 動いたら true（飛ぶ数がステージの数でちょうど割り切れて、同じステージに戻るときは false）
			 */
			bool Jump(int direction);

			/** 選んでいるステージ（StageCatalog の表の番号） */
			int GetSelectedStage() const { return m_selected; }
			/** ステージの数 */
			int GetStageCount() const { return m_stageCount; }


		private:
			/** ステージの数 */
			int m_stageCount;
			/** 選んでいるステージ */
			int m_selected;
		};
	}
}
