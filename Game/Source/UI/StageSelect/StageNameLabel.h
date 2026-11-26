/**
 * @file StageNameLabel.h
 * @brief 「STAGE 5」（最後のステージは「STAGE FINAL」）を横1行で出す文字
 * @details 画像はステージが始まる前の表示と同じ物（Assets/sprite/stage_intro/ の STAGE・FINAL・数字）。
 *          けたごとに 0〜9 の数字を全部読み込んでおき、SetStage で出す数字を選ぶだけにしている
 *          （画像を読み込み直さないので、何回ステージを変えてもよい。2けた＝99 まで）。
 */
#pragma once
#include "Source/UI/UISprite.h"
#include <array>


namespace app
{
	namespace ui
	{
		/**
		 * @brief 「STAGE 5」「STAGE FINAL」の文字
		 */
		class StageNameLabel
		{
		public:
			StageNameLabel();
			~StageNameLabel();
			StageNameLabel(const StageNameLabel&) = delete;
			StageNameLabel& operator=(const StageNameLabel&) = delete;

			/**
			 * @brief 画像を読み込む（1回だけ）
			 * @param height 文字の高さ
			 */
			void Init(float height);
			/**
			 * @brief 出すステージを決める
			 * @param stageIndex StageCatalog の表の番号（最後のステージなら FINAL）
			 */
			void SetStage(int stageIndex);
			/**
			 * @brief 置く（左端をそろえる）
			 * @param left 左端の X（拡大率をかけても左端は動かない）
			 */
			void PlaceLeft(float left, float y, float scale, const Vector4& color);
			/** 描く */
			void Draw(RenderContext& rc);


		private:
			static constexpr int MAX_DIGIT_COUNT = 2;
			static constexpr int DIGIT_KIND_COUNT = 10;

			/** 「STAGE」の文字 */
			UISprite m_label;
			/** 「FINAL」の文字 */
			UISprite m_final;
			/** 数字（[けた][数字]。上のけたから） */
			std::array<std::array<UISprite, DIGIT_KIND_COUNT>, MAX_DIGIT_COUNT> m_digits;
			/** 出す数字（けたごと） */
			std::array<int, MAX_DIGIT_COUNT> m_shownDigits;
			/** 使っている数字のけた数（FINAL のときは 0） */
			int m_digitCount;
			/** FINAL を出すか */
			bool m_isFinal;
		};
	}
}
