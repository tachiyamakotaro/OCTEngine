/**
 * @file StageSelectMenu.h
 * @brief ステージセレクト画面（クリアしたかに関係なく、どのステージからでも遊べる）
 * @details 展示会や作品の提出で、好きなステージをすぐに見せられるようにするための画面。タイトルメニューの SELECT から開く。
 *          操作
 *              左右 … ステージを選ぶ（ページの端を越えると、となりのページへ）
 *              LB / RB（キーボードは B / 7）… ページをめくる
 *              A … そのステージから遊ぶ（Update が Decide を返す）
 *              B … 閉じる（Update が Cancel を返す）
 *          1ページに5ステージずつ並べる。ページの数はステージの数（StageCatalog の STAGE_COUNT）から決まるので、
 *          StageCatalog の表にステージを足す・減らすだけで、札とページが増える・減る（ここは直さなくてよい）。
 *          ページとカーソルの計算は StageSelectCursor、1ステージぶんの見た目は StageCard。
 *          背景（半透明の黒）もこのクラスが描くので、呼んだ側は背景を描かなくてよい。
 */
#pragma once
#include "Source/UI/StageSelect/StageCard.h"
#include "Source/UI/StageSelect/StageSelectCursor.h"
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
			/** 開く（今遊ぶステージ（前に遊んだステージ）を選んだ状態にする） */
			void Open();
			/**
			 * @brief 入力を見て操作する（開いている間、毎フレーム呼ぶ）
			 * @param input 呼んだ側のカーソル操作（Update() 済みのもの）
			 */
			Result Update(const MenuInput& input);
			/** 描く（開いている間だけ呼ぶ） */
			void Render(RenderContext& rc);

			/** 選んでいるステージ（StageCatalog の表の番号） */
			int GetSelectedStage() const { return m_cursor.GetSelectedStage(); }


		private:
			/** 札とページの印の見た目を更新する */
			void UpdateVisual();
			/** 今のページの札を並べる（ページのステージの数に合わせて、全体を画面の真ん中に） */
			void PlaceCards();
			/** ページの印（ひし形）を並べる */
			void PlacePageMarks();
			/** 動かない画像（見出し・案内）を置く */
			void PlaceFixedSprites();


		private:
			/** 選んでいるステージとページ */
			StageSelectCursor m_cursor;
			/** 演出用のタイマー（秒。選んでいる札を脈打たせる。カーソルを動かすと 0 に戻る） */
			float m_timer;

			// ---- 画像 ----
			/** 背景（フェードの黒い画像を半透明にして使う） */
			UISprite m_background;
			/** 見出し（STAGE SELECT） */
			UISprite m_title;
			/**
			 * 札（ステージの数だけ。表の順）
			 * 画像は中に GPU の資源を持っていてコピーできないので、unique_ptr で持つ（vector が増えるときに中身が動かないように）
			 */
			std::vector<std::unique_ptr<StageCard>> m_cards;
			/** ページの印（ページの数だけ） */
			std::vector<std::unique_ptr<UISprite>> m_pageMarks;
			/** ページをめくる案内（LB / RB。2ページ以上あるときだけ出す） */
			UISprite m_buttonLB;
			UISprite m_buttonRB;
			/** 操作の案内（A OK / B BACK） */
			UISprite m_buttonA;
			UISprite m_labelOk;
			UISprite m_buttonB;
			UISprite m_labelBack;
		};
	}
}
