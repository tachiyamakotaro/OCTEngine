/**
 * @file StageIntroUI.h
 * @brief ステージが始まる前の表示（暗い画面に「STAGE 3」（最後のステージは「STAGE FINAL」）と、置いてあるギミックのアイコン）
 * @details 見た目は全部画像（UISprite）。画像は Assets/sprite/stage_intro/（STAGE・FINAL の文字と数字 0〜9）。
 *          番号は数字の画像を組み合わせて出すので、ステージを足しても画像を作らなくてよい。
 *          背景は SceneManager のフェードと同じ暗い画像（Assets/sprite/fade.dds）なので、フェードとつながって見える。
 *          ギミックのアイコンは、そのステージに置いてある物だけを GimmickIconTable.h の表の順に並べる。
 *          あとで制限時間などを足すときは、このクラスに画像を足す。
 */
#pragma once
#include "Source/UI/GimmickIconTable.h"
#include "Source/UI/UISprite.h"
#include <array>


namespace app
{
	namespace stage
	{
		class StageGimmickTypes;
	}

	namespace ui
	{
		/**
		 * @brief ステージが始まる前の表示
		 */
		class StageIntroUI
		{
		public:
			StageIntroUI();
			~StageIntroUI();
			StageIntroUI(const StageIntroUI&) = delete;
			StageIntroUI& operator=(const StageIntroUI&) = delete;

			/**
			 * @brief 「STAGE 3」を用意する（シーンの Start で1回）
			 * @param stageNumber 出すステージの番号（1 から。2けたまで）
			 */
			void InitNumber(int stageNumber);
			/**
			 * @brief 「STAGE FINAL」を用意する（最後のステージ。シーンの Start で1回）
			 */
			void InitFinal();
			/**
			 * @brief ギミックのアイコンを用意する（置いてある物だけ。シーンの Start で1回）
			 * @param types そのステージに置いてあるギミックの種類
			 */
			void InitGimmickIcons(const stage::StageGimmickTypes& types);
			/** 描く */
			void Render(RenderContext& rc);

		private:
			/** 出せる数字のけた数 */
			static constexpr int MAX_DIGIT_COUNT = 2;
			/** 背景と「STAGE」の文字を用意する（InitNumber / InitFinal の最初） */
			void InitCommon();
			/**
			 * @brief 「STAGE」と、その右に出す物の全体が、画面の真ん中に来るように並べる
			 * @param valueWidth 右に出す物（数字・FINAL）の幅
			 * @return 右に出す物の左端の X
			 */
			float LayoutLabel(float valueWidth);

		private:
			/** 背景（画面いっぱいの暗い画像） */
			UISprite m_background;
			/** 「STAGE」の文字 */
			UISprite m_label;
			/** 「FINAL」の文字（最後のステージ） */
			UISprite m_final;
			/** 数字（上のけたから） */
			std::array<UISprite, MAX_DIGIT_COUNT> m_digits;
			/** 使っている数字のけた数（FINAL のときは 0） */
			int m_digitCount;
			/** FINAL を出すか */
			bool m_isFinal;
			/** ギミックのアイコン（表の数だけ用意しておき、使う分だけ読み込む） */
			std::array<UISprite, GIMMICK_ICON_COUNT> m_icons;
			/** 出すアイコンの数 */
			int m_iconCount;
		};
	}
}
