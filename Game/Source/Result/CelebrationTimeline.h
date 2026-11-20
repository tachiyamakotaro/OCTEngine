#pragma once
#include "Source/Result/ResultParameter.h"


/**
 * @file CelebrationTimeline.h
 * @brief 宝物の間の演出の時間の流れ（何秒に何が起きるか）
 */
namespace app
{
	namespace result
	{
		/**
		 * @brief 宝物の間の演出の時間の流れ
		 * @details ・毎フレーム Update で時間を進め、そのフレームに起きたこと（ふたが開いた など）を返す。音を鳴らすのはシーン
		 *          ・どこまで進んだかを 0〜1 の割合で答える（GetLidOpenRate など）。
		 *            宝箱を動かすのは TreasureRenderer、プレイヤーを動かすのは ResultPlayerModel（このクラスは時間を数えるだけ）
		 *          ・A ボタンで最後まで飛ばせる（SkipToEnd）
		 */
		class CelebrationTimeline
		{
		public:
			/** そのフレームに起きたこと */
			struct Events
			{
				bool isLidOpened = false;	//!< ふたが開き始めた
				bool isGemShown = false;	//!< 宝石が浮かび始めた
				bool isHopped = false;		//!< 跳ねた
				bool isLogoShown = false;	//!< GAME CLEAR が出始めた
			};

		public:
			CelebrationTimeline() = default;
			~CelebrationTimeline() = default;

			void Init(const CelebrationTiming& timing = CelebrationTiming());
			/** 時間を進める（そのフレームに起きたことを返す） */
			Events Update(float deltaTime);
			/** 最後まで飛ばす（飛ばした分の出来事は返さない） */
			void SkipToEnd();

			/** 始まってからの時間 */
			float GetTime() const { return m_time; }
			/** 丸の閉じ具合（1 = 真っ暗 → 0 = 開いた） */
			float GetIrisCloseRate() const;
			/** 走った割合（0 = 走り始めの位置 〜 1 = 止まる位置。止まる前はゆっくりになる） */
			float GetRunRate() const;
			/** 走っているか */
			bool IsRunning() const;
			/**
			 * @brief 今跳ねている回数目（0 から。跳ねていなければ -1）
			 */
			int GetHopIndex() const;
			/** 今の跳ねる動きの割合（0 = 踏み切り 〜 1 = 着地。跳ねていなければ 0） */
			float GetHopRate() const;
			/** 喜び始めてからの割合（0〜1。振り向くのに使う） */
			float GetCelebrateRate() const;
			/** ふたの開き具合（0〜1） */
			float GetLidOpenRate() const;
			/** 宝石の浮かび具合（0〜1） */
			float GetGemRiseRate() const;
			/** GAME CLEAR の出具合（0〜1） */
			float GetLogoRate() const;
			/** タイトルへ戻れるか（ボタンの案内が出ているか） */
			bool CanLeave() const { return m_time >= m_timing.promptStart; }
			/** ボタンの案内が出てからの時間（点滅に使う。出ていなければ 0） */
			float GetPromptTime() const;
			/** カメラの寄り具合（0〜1） */
			float GetDollyRate() const;

		private:
			/** start から duration かけて 0 → 1（範囲の外は 0 か 1） */
			float RateBetween(float start, float duration) const;
			/** 前のフレームから今のフレームの間に、その時間を通ったか */
			bool IsPassed(float previous, float time) const { return previous < time && time <= m_time; }

		private:
			CelebrationTiming m_timing;
			float m_time = 0.0f;
		};
	}
}
