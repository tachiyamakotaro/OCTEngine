/**
 * @file StageSelectMenu.h
 * @brief ステージセレクト画面（クリアしたかに関係なく、どのステージからでも遊べる）
 * @details 展示会や作品の提出で、好きなステージをすぐに見せられるようにするための画面。
 *          タイトルの START → どこから遊ぶか（StartStageMenu）の「STAGE SELECT」から開く。
 *          操作
 *              左右 … ステージを1つずつ選ぶ（最後のステージの次は最初のステージ。逆も）
 *              LB / RB（キーボードは B / 7）… 5つずつ飛ぶ（端を越えたら反対側へ回り込む）。押すと LB / RB の絵がポンと大きくなって戻る
 *              A … そのステージから遊ぶ（Update が Decide を返す）
 *              B … 閉じる（Update が Cancel を返す）
 *          ページはない。全部の札を1列に並べ、選んでいる札が真ん中に来るように列全体が横にすべって動く。
 *          列は輪になっていて、FINAL の右には STAGE 1 が並ぶ（マリオWiiのワールド選択のように、ぐるっと回れる）。
 *          真ん中の札が一番大きく、離れるほど小さく、ウィンドウの端のほうは薄くなって消える（StageCarousel）。
 *          札の下に、ステージの数だけひし形を並べ、今どこを選んでいるかを出す（今のステージは黄色で大きい）。
 *          StageCatalog の表にステージを足す・減らすだけで、札とひし形が増える・減る（ここは直さなくてよい）。
 *          選んでいるステージの計算は StageSelectCursor、札の置き方は StageCarousel、1ステージぶんの見た目は StageCard。
 *          見た目：見出しの帯のあるウィンドウ（MenuWindow）に、見出し・札・ひし形・LB / RB を入れる。文字には暗い縁を付ける。
 *          右下の操作の案内（Ⓐ OK　Ⓑ BACK）も小さなウィンドウ（MenuGuide）。
 *          ウィンドウは中身（札・ひし形など）ごと拡大縮小で出し入れする（UIPopupAnimation）。Open で大きくなって出て、Close で小さくなって消える。
 *          閉じている途中も描くので、呼んだ側は毎フレーム Animate と Render を呼ぶ（見えていないときは何もしない）。
 *          後ろの 3D の絵のぼかし・暗さは、呼んだ側（TitleMenu の MenuBackdrop）がする。
 */
#pragma once
#include "Source/UI/MenuGuide.h"
#include "Source/UI/MenuWindow.h"
#include "Source/UI/StageSelect/StageCard.h"
#include "Source/UI/StageSelect/StageCarousel.h"
#include "Source/UI/StageSelect/StageSelectCursor.h"
#include "Source/UI/Animation/UIPopupAnimation.h"
#include "Source/UI/UISprite.h"
#include <memory>
#include <vector>


namespace app
{
	namespace ui
	{
		class MenuInput;


		/**
		 * @brief ステージセレクト画面
		 */
		class StageSelectMenu
		{
		public:
			/** 操作の結果 */
			enum class Result
			{
				None,	//!< 何もない
				Decide,	//!< ステージを決めた（GetSelectedStage で選んだステージがわかる）
				Cancel,	//!< 閉じる
			};


		public:
			StageSelectMenu();
			~StageSelectMenu();
			StageSelectMenu(const StageSelectMenu&) = delete;
			StageSelectMenu& operator=(const StageSelectMenu&) = delete;

			/** 画像を読み込む（全部のステージの札を作る。ステージの JSON も読むので、シーンの Start で1回） */
			void Init();
			/**
			 * @brief 開く
			 * @param initialStage 最初に選んでおくステージ（StageCatalog の表の番号。タイトルからは、続きのステージ（なければ最初のステージ））
			 */
			void Open(int initialStage);
			/** 閉じる（ウィンドウが中身ごと小さくなって消える） */
			void Close();
			/**
			 * @brief 入力を見て操作する（開いている間、毎フレーム呼ぶ）
			 * @param input 呼んだ側のカーソル操作（Update() 済みのもの）
			 */
			Result Update(const MenuInput& input);
			/** 開閉のアニメーションを進めて、置き直す（開いていないときも毎フレーム呼ぶ） */
			void Animate();
			/** 描く（見えていないときは何もしない） */
			void Render(RenderContext& rc);
			/** 見えているか（開いている・開いている途中・閉じている途中） */
			bool IsVisible() const { return m_popup.IsVisible(); }

			/** 選んでいるステージ（StageCatalog の表の番号） */
			int GetSelectedStage() const { return m_cursor.GetSelectedStage(); }


		private:
			/** 全部の画像を置き直す（ウィンドウの拡大率・濃さもかける） */
			void UpdateVisual();
			/** 札を並べる（StageCarousel が決めた位置・大きさ・濃さ。選んでいる札は黄色で脈打つ） */
			void PlaceCards(const UIGroupTransform& group);
			/** ひし形を並べる（全体をウィンドウの真ん中に。今のステージは黄色で大きい） */
			void PlaceStageMarks(const UIGroupTransform& group);
			/** LB / RB を置く（押したあとしばらくは、ポンと大きくなって戻る） */
			void PlaceJumpButtons(const UIGroupTransform& group);
			/** LB / RB を押してからの時間から、LB / RB の絵の拡大率を決める */
			static float GetButtonPopScale(float timeSincePressed);


		private:
			/** 選んでいるステージ */
			StageSelectCursor m_cursor;
			/** 札の並べ方（横に流れる） */
			StageCarousel m_carousel;
			/** 演出用のタイマー（秒。選んでいる札を脈打たせる。カーソルを動かすと 0 に戻る） */
			float m_timer;
			/** ウィンドウの出し入れ（拡大縮小） */
			UIPopupAnimation m_popup;
			/** LB / RB を押してからの時間（秒。押した絵をポンと大きくする） */
			float m_timeSinceLB;
			float m_timeSinceRB;

			// ---- 画像 ----
			/** ウィンドウ（見出しの帯つき） */
			MenuWindow m_window;
			/** 見出し（STAGE SELECT） */
			UISprite m_title;
			/**
			 * 札（ステージの数だけ。表の順）
			 * 画像は中に GPU の資源を持っていてコピーできないので、unique_ptr で持つ（vector が増えるときに中身が動かないように）
			 */
			std::vector<std::unique_ptr<StageCard>> m_cards;
			/** 今どこを選んでいるかのひし形（ステージの数だけ） */
			std::vector<std::unique_ptr<UISprite>> m_stageMarks;
			/** 5つずつ飛ぶ案内（LB / RB。ひし形の両わき） */
			UISprite m_buttonLB;
			UISprite m_buttonRB;
			/** 操作の案内（Ⓐ OK　Ⓑ BACK） */
			MenuGuide m_guide;
		};
	}
}
