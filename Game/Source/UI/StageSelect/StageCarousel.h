/**
 * @file StageCarousel.h
 * @brief ステージセレクトの札を「横に流れる」ように並べる計算（画像は持たない）
 * @details ページはやめて、全部の札を1列に並べる。選んでいる札が真ん中に来るように、列全体が横にすべって動く。
 *          列は輪のようにつながっている（マリオWiiのワールド選択のように、FINAL の右に STAGE 1、STAGE 1 の左に FINAL が来る）。
 *          FINAL から右へ動くと、巻き戻らずに、そのまま右へ1枚分流れて STAGE 1 が真ん中に来る。
 *            ・真ん中の札が一番大きく、離れるほど小さくなる（間も詰まる）
 *            ・真ん中から2枚より外は、だんだん薄くなって消える（ウィンドウの端でフェードアウト）
 *          「どこまで流れたか」（m_scroll）は、選んでいるステージの番号へ少しずつ近づく小数（0 〜 ステージの数 の輪の上）。
 *          近づくときは、輪の近いほうの向きへ流れる。
 *          札の位置は「その札の番号 − m_scroll」を輪の上の一番近い距離にした物（真ん中からの距離。小数）から決めるので、
 *          動いている途中も、札の大きさ・間・濃さがなめらかに変わる。
 *
 *          距離 d（札の数）と見た目（拡大率 1 の札の幅を W とする）
 *            拡大率   scale(d) = CENTER_SCALE − SCALE_STEP × d（SCALE_DISTANCE より外は同じ）
 *            真ん中からの X … となりどうしの札のすき間が CARD_GAP になるように、拡大率から計算する
 *            濃さ     d が FADE_START まで 1、FADE_END で 0
 */
#pragma once
#include "Source/UI/StageSelect/StageCard.h"


namespace app
{
	namespace ui
	{
		/**
		 * @brief 横に流れる札の並べ方
		 */
		class StageCarousel
		{
		public:
			/**
			 * @param center 選んでいる札（真ん中の札）の真ん中
			 * @param stageCount ステージの数（輪の長さ。StageCatalog の STAGE_COUNT）
			 */
			StageCarousel(const Vector2& center, int stageCount);

			/** すぐにそのステージを真ん中にする（開いたとき。すべらせない） */
			void SnapTo(int stageIndex);
			/**
			 * @brief 真ん中にしたいステージへ、少しずつ流す（毎フレーム呼ぶ）
			 * @param stageIndex 真ん中にしたいステージ（選んでいるステージ）
			 */
			void Update(int stageIndex);
			/**
			 * @brief そのステージの札の置き方（位置・拡大率・濃さ）
			 * @details 濃さが 0 の札は描かなくてよい（IsVisible）
			 */
			StageCardPose GetPose(int stageIndex) const;
			/** そのステージの札が見えるか（濃さが 0 より大きいか） */
			bool IsVisible(int stageIndex) const;


		private:
			/** 真ん中からの距離（札の数。右が +。輪の上で一番近い向き） */
			float GetDistance(int stageIndex) const;
			/** 輪の上の差を、一番近い向きの差にする（−ステージの数 ÷ 2 〜 +ステージの数 ÷ 2） */
			float WrapDifference(float difference) const;
			/** 真ん中からの距離（0 以上）での拡大率 */
			static float GetScale(float distance);
			/** 真ん中からの距離（0 以上）での、真ん中からの X */
			static float GetOffsetX(float distance);
			/** 真ん中からの距離（0 以上）での濃さ */
			static float GetAlpha(float distance);


		private:
			/** 真ん中の札の真ん中 */
			Vector2 m_center;
			/** ステージの数（輪の長さ） */
			float m_stageCount;
			/** どこまで流れたか（真ん中にある所のステージの番号。小数。0 〜 ステージの数） */
			float m_scroll;
		};
	}
}
