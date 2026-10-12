/**
 * @file MirrorInteractor.h
 * @brief プレイヤーによる反射鏡の操作（回転・移動）
 * @details 操作方法
 *          ・近くの鏡の前でアクション（Bボタン）… 1段回転
 *              右を向いていれば時計回り、左を向いていれば反時計回り
 *          ・近くの鏡の前でつかむ（RB1）を押したまま左右に歩く … 鏡を引きずって動かす
 *
 *          プレイヤーのステートには手を入れず、プレイヤーの「コマンド」と位置を見て鏡を動かす。
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
		 * @brief 反射鏡の操作
		 */
		class MirrorInteractor
		{
		public:
			/** 鏡を操作できる横の距離（体の中心から鏡の中心まで） */
			float interactRangeX = 90.0f;
			/** 鏡を操作できる縦の距離（足元から鏡の中心まで） */
			float interactRangeY = 160.0f;


		public:
			/**
			 * @brief 毎フレームの更新（キャラクターの Update のあとに呼ぶ）
			 * @param stage ステージ
			 * @param character 操作するキャラクター（本物の世界にいるプレイヤー）
			 */
			void Update(Stage& stage, const actor::CharacterBase& character);
			/**
			 * @brief 操作対象の鏡（なければ -1）。UI で「Bで回転」などを出すときに使う
			 */
			inline int GetFocusedMirrorIndex() const { return m_focusedIndex; }
			/**
			 * @brief 鏡を引きずっているか
			 */
			inline bool IsDragging() const { return m_isDragging; }
			/**
			 * @brief このフレームに鏡を回したか（音や演出のきっかけに使う）
			 */
			inline bool IsRotatedThisFrame() const { return m_isRotatedThisFrame; }
			/**
			 * @brief このフレームに鏡をつかんだか（引きずり始めた瞬間）
			 */
			inline bool IsGrabbedThisFrame() const { return m_isGrabbedThisFrame; }
			/**
			 * @brief このフレームに鏡を動かした距離（引きずって実際に動いた分。動かなければ 0）
			 */
			inline float GetMovedDistanceThisFrame() const { return m_movedDistanceThisFrame; }
			/**
			 * @brief 状態を最初に戻す
			 */
			void Reset();


		public:
			MirrorInteractor();


		private:
			/** 操作できる一番近い鏡を探す */
			int FindNearestMirror(const Stage& stage, const actor::CharacterBase& character) const;


		private:
			/** 操作対象の鏡 */
			int m_focusedIndex;
			/** 引きずっているか */
			bool m_isDragging;
			/** 引きずり始めたときの、鏡の中心とキャラクターのX座標の差 */
			float m_dragOffsetX;
			/** このフレームに回したか */
			bool m_isRotatedThisFrame;
			/** このフレームにつかんだか */
			bool m_isGrabbedThisFrame;
			/** このフレームに動かした距離 */
			float m_movedDistanceThisFrame;
		};
	}
}
