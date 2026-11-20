#pragma once
#include "Source/UI/UISprite.h"


/**
 * @file ResultUI.h
 * @brief 宝物の間の 2D の表示（GAME CLEAR のロゴ・CONGRATULATIONS!・A でタイトルへの案内）
 */
namespace app
{
	namespace result
	{
		class CelebrationTimeline;


		/**
		 * @brief 宝物の間の 2D の表示
		 * @details ・GAME CLEAR は大きい所から弾むように縮んで出てくる。CONGRATULATIONS! はその下に少し遅れて出る
		 *          ・タイトルへ戻れるようになったら、右下に「A TITLE」を点滅させる
		 *          ・出るタイミングは CelebrationTimeline から受け取る（このクラスは時間を数えない）
		 *          ・座標は画面の真ん中が原点、上が +Y（画面は 1920x1080）
		 */
		class ResultUI
		{
		public:
			ResultUI() = default;
			~ResultUI() = default;

			void Init();
			/** 演出の進み具合に合わせて置く（毎フレーム） */
			void Update(const CelebrationTimeline& timeline);
			void Render(RenderContext& rc);

		private:
			/** GAME CLEAR */
			ui::UISprite m_logo;
			/** CONGRATULATIONS! */
			ui::UISprite m_congratulations;
			/** A ボタンの絵 */
			ui::UISprite m_buttonA;
			/** TITLE の文字 */
			ui::UISprite m_labelTitle;
			/** ロゴを描くか */
			bool m_isLogoVisible = false;
			/** 案内を描くか */
			bool m_isPromptVisible = false;
		};
	}
}
