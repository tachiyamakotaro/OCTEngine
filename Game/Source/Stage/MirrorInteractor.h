/**
 * @file MirrorInteractor.h
 * @brief プレイヤーによる反射鏡の操作（回転・移動）
 * @details 操作方法
 *          ・近くの鏡の前で RB … 右に1段回転（時計回り）
 *          ・近くの鏡の前で LB … 左に1段回転（反時計回り）
 *              プレイヤーの向きには関係しない。LB と RB を同じフレームに押したときは回さない
 *          ・近くの鏡の前でつかむ（Bボタン）を押したまま左右に歩く … 鏡を引きずって動かす
 *              レールの端や地形で鏡が止まり、キャラクターが鏡から離れたら、手を離す
 *              （B を押したままでも、ふつうの歩き方に戻る。もう一度つかむには B を押し直す）
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
		struct CharacterCommand;
	}


	namespace stage
	{
		class Stage;
		class LightMirror;


		/**
		 * @brief 反射鏡の操作
		 */
		class MirrorInteractor
		{
		public:
			/** 鏡を回した向き */
			enum class RotateDirection
			{
				None,	// 回していない
				Left,	// 左（反時計回り。LB）
				Right,	// 右（時計回り。RB）
			};
			/** 鏡を操作できる横の距離（体の中心から鏡の中心まで） */
			float interactRangeX = 90.0f;
			/** 鏡を操作できる縦の距離（足元から鏡の中心まで） */
			float interactRangeY = 160.0f;
			/** 鏡がついてこられなくなって（レールの端・地形）、つかんだ位置からこれだけ離れたら手を離す */
			float releaseDistance = 30.0f;


		public:
			/**
			 * @brief 毎フレームの更新（キャラクターの Update のあとに呼ぶ）
			 * @param stage ステージ
			 * @param character 操作するキャラクター（本物の世界にいるプレイヤー）
			 */
			void Update(Stage& stage, const actor::CharacterBase& character);
			/**
			 * @brief 操作対象の鏡が、全ギミックの何番目か（なければ -1）。UI で「Bで回転」などを出すときに使う
			 * @details stage.GetGimmicks().Get(index) で取り出し、LightMirror に dynamic_cast して使う
			 */
			inline int GetFocusedMirrorIndex() const { return m_focusedIndex; }
			/**
			 * @brief 鏡を引きずっているか
			 */
			inline bool IsDragging() const { return m_isDragging; }
			/**
			 * @brief 引きずり始めたときの、鏡の中心とキャラクターのX座標の差（正なら鏡はキャラクターの右側）
			 */
			inline float GetDragOffsetX() const { return m_dragOffsetX; }
			/**
			 * @brief このフレームに鏡を回したか（音や演出のきっかけに使う）
			 */
			inline bool IsRotatedThisFrame() const { return m_rotatedDirection != RotateDirection::None; }
			/**
			 * @brief このフレームに鏡を回した向き（回していなければ None）。UI で押したボタンを跳ねさせるときに使う
			 */
			inline RotateDirection GetRotatedDirectionThisFrame() const { return m_rotatedDirection; }
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
			/** コマンドから、回す向きを決める（LB と RB を同時に押したときは回さない） */
			static RotateDirection ToRotateDirection(const actor::CharacterCommand& command);
			/** 鏡がついてこられずに、キャラクターがつかんだ位置から離れたか */
			bool IsLeftBehind(const LightMirror& mirror, float characterX) const;


		private:
			/** 操作対象の鏡（全ギミックの何番目か） */
			int m_focusedIndex;
			/** 引きずっているか */
			bool m_isDragging;
			/** 引きずり始めたときの、鏡の中心とキャラクターのX座標の差 */
			float m_dragOffsetX;
			/** このフレームに回した向き */
			RotateDirection m_rotatedDirection;
			/** このフレームにつかんだか */
			bool m_isGrabbedThisFrame;
			/** このフレームに動かした距離 */
			float m_movedDistanceThisFrame;
			/** 離れて手を離したあと、つかむ（B）を一度離すのを待っているか（押したままだと、すぐつかみ直してしまうため） */
			bool m_isWaitingGrabRelease;
		};
	}
}
