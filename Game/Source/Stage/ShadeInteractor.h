/**
 * @file ShadeInteractor.h
 * @brief プレイヤーによる日よけ板の操作（つかんで左右に動かす）
 * @details 操作方法
 *          ・近くの日よけ板の前（下）でつかむ（RB1）を押したまま左右に歩く … 板を引きずって動かす
 *          反射鏡の操作（MirrorInteractor）と同じボタンなので、同時につかまないようにシーン側で順番を決める。
 *            ・反射鏡を引きずっている間は、板をつかまない（Update の canGrab を false にする）
 *            ・板を引きずっている間は、反射鏡を操作しない（MirrorInteractor を Reset する）
 *          プレイヤーのステートには手を入れず、プレイヤーの「コマンド」と位置を見て板を動かす。
 *          プレイヤーの Update が終わったあとに毎フレーム呼ぶこと。
 */
#pragma once


namespace app
{
	namespace actor
	{
		class CharacterBase;
	}


	namespace stage
	{
		class Stage;


		/**
		 * @brief 日よけ板の操作
		 */
		class ShadeInteractor
		{
		public:
			/** 板をつかめる横の距離（体の中心から板の端まで。板の幅の中に立っていれば 0） */
			float interactRangeX = 60.0f;
			/** 板をつかめる縦の距離（足元から板の下の面まで） */
			float interactRangeY = 160.0f;


		public:
			/**
			 * @brief 毎フレームの更新（キャラクターの Update のあとに呼ぶ）
			 * @param stage ステージ
			 * @param character 操作するキャラクター
			 * @param canGrab 新しく板をつかんでよいか（反射鏡を引きずっている間は false）
			 */
			void Update(Stage& stage, const actor::CharacterBase& character, bool canGrab);
			/**
			 * @brief 操作対象の板が、全ギミックの何番目か（なければ -1）。UI で「RBでつかむ」などを出すときに使う
			 * @details stage.GetGimmicks().Get(index) で取り出し、ShadeBoard に dynamic_cast して使う
			 */
			inline int GetFocusedIndex() const { return m_focusedIndex; }
			/** @brief 板を引きずっているか */
			inline bool IsDragging() const { return m_isDragging; }
			/** @brief このフレームに板をつかんだか（引きずり始めた瞬間） */
			inline bool IsGrabbedThisFrame() const { return m_isGrabbedThisFrame; }
			/** @brief このフレームに板を動かした距離（引きずって実際に動いた分。動かなければ 0） */
			inline float GetMovedDistanceThisFrame() const { return m_movedDistanceThisFrame; }
			/** @brief 状態を最初に戻す */
			void Reset();


		public:
			ShadeInteractor();


		private:
			/** 操作できる一番近い板を探す（なければ -1） */
			int FindNearestBoard(const Stage& stage, const actor::CharacterBase& character) const;


		private:
			/** 操作対象の板（全ギミックの何番目か） */
			int m_focusedIndex;
			/** 引きずっているか */
			bool m_isDragging;
			/** 引きずり始めたときの、板の中心とキャラクターのX座標の差 */
			float m_dragOffsetX;
			/** このフレームにつかんだか */
			bool m_isGrabbedThisFrame;
			/** このフレームに動かした距離 */
			float m_movedDistanceThisFrame;
		};
	}
}
