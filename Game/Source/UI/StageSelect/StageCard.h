/**
 * @file StageCard.h
 * @brief ステージセレクトに並べる、1ステージぶんの札（「STAGE」と番号（最後は FINAL）と、置いてあるギミックのアイコン）
 * @details 画像はステージが始まる前の表示（StageIntroUI）と同じ物（Assets/sprite/stage_intro/）。
 *          番号は数字の画像を組み合わせるので、ステージを足しても画像を作らなくてよい。
 *          ギミックのアイコンは、そのステージの JSON の "type" を見て GimmickIconTable.h の表の順に並べる。
 *          札の枠と地は Assets/sprite/stage_select/card.dds（白い角丸の四角）に色を付けている。
 */
#pragma once
#include "Source/UI/GimmickIconTable.h"
#include "Source/UI/UISprite.h"
#include <array>


namespace app
{
	namespace ui
	{
		/**
		 * @brief 札を置く所と大きさ・濃さ（StageCarousel が決める）
		 */
		struct StageCardPose
		{
			/** 札の真ん中 */
			Vector2 center;
			/** 拡大率 */
			float scale = 1.0f;
			/** 濃さ（0 = 見えない 〜 1）。枠・地・文字・アイコンの全部にかける */
			float alpha = 1.0f;
		};


		/**
		 * @brief 1ステージぶんの札
		 */
		class StageCard
		{
		public:
			/** 札の大きさ（拡大率 1 のとき） */
			static constexpr float WIDTH = 300.0f;
			static constexpr float HEIGHT = 400.0f;


		public:
			StageCard();
			~StageCard();
			StageCard(const StageCard&) = delete;
			StageCard& operator=(const StageCard&) = delete;

			/**
			 * @brief そのステージの札を用意する（1回だけ呼ぶ）
			 * @param stageIndex StageCatalog の表の番号
			 */
			void Init(int stageIndex);
			/** 選んでいない札として置く（灰色） */
			void PlaceNormal(const StageCardPose& pose);
			/** 選んでいる札として置く（枠と文字を黄色にする。大きさ・脈打たせるのは呼ぶ側） */
			void PlaceSelected(const StageCardPose& pose);
			/** 描く */
			void Draw(RenderContext& rc);


		private:
			/** 札の色と大きさ */
			struct Look
			{
				Vector4 frameColor;
				Vector4 textColor;
				Vector4 iconColor;
			};

			/** 番号（1〜99）を用意する */
			void InitNumber(int stageNumber);
			/** FINAL を用意する */
			void InitFinal();
			/** そのステージに置いてあるギミックのアイコンを用意する */
			void InitIcons(int stageIndex);
			/** 中身をまとめて置く（札の真ん中からの位置に拡大率をかけ、色に濃さをかける） */
			void Place(const StageCardPose& pose, const Look& look);


		private:
			static constexpr int MAX_DIGIT_COUNT = 2;

			/** 枠（地より少し大きい角丸の四角） */
			UISprite m_frame;
			/** 地（暗い角丸の四角） */
			UISprite m_panel;
			/** 「STAGE」の文字 */
			UISprite m_label;
			/** 「FINAL」の文字（最後のステージ） */
			UISprite m_final;
			/** 数字（上のけたから） */
			std::array<UISprite, MAX_DIGIT_COUNT> m_digits;
			/** 数字の、札の真ん中からの X（拡大率 1） */
			std::array<float, MAX_DIGIT_COUNT> m_digitOffsetX;
			/** 使っている数字のけた数（FINAL のときは 0） */
			int m_digitCount;
			/** FINAL を出すか */
			bool m_isFinal;
			/** ギミックのアイコン（表の数だけ用意しておき、使う分だけ読み込む） */
			std::array<UISprite, GIMMICK_ICON_COUNT> m_icons;
			/** 出すアイコンの数 */
			int m_iconCount;
			/** アイコンどうしの間（中心から中心。アイコンが多くても札からはみ出ないように Init で決める） */
			float m_iconSpacing;
		};
	}
}
