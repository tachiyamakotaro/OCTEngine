/**
 * @file StartStageMenu.h
 * @brief タイトルの START を押したあとに出す「どこから遊ぶか」を選ぶ画面
 * @details 3つから選ぶ（上から）。
 *            ・NEW GAME     … 最初のステージから（始めから）
 *            ・CONTINUE     … 続きのステージから（右に「STAGE 5」のように続きのステージを出す）。続きの記録がないときは灰色で選べない
 *            ・STAGE SELECT … ステージセレクト画面を開く（クリアしたかに関係なく、どのステージでも選べる）
 *          操作
 *              上下（左右でも可）… 選ぶ（選べない項目は飛ばす）
 *              A … 決定（Update が NewGame / Continue / StageSelect を返す）
 *              B … 閉じる（Update が Cancel を返す）
 *          見出しはタイトルの START のボタンと同じ画像。
 *          見た目：見出しの帯のあるウィンドウ（MenuWindow）に入れて、文字には暗い縁を付ける。
 *          右下の操作の案内（Ⓐ OK　Ⓑ BACK）も小さなウィンドウ（MenuGuide）。
 *          ウィンドウは拡大縮小で出し入れする（UIPopupAnimation）。Open / Reopen で大きくなって出て、Close で小さくなって消える。
 *          閉じている途中も描くので、呼んだ側は毎フレーム Animate と Render を呼ぶ（見えていないときは何もしない）。
 *          後ろの 3D の絵のぼかし・暗さは、呼んだ側（TitleMenu の MenuBackdrop）がする。
 */
#pragma once
#include "Source/UI/MenuGuide.h"
#include "Source/UI/MenuWindow.h"
#include "Source/UI/StageSelect/StageNameLabel.h"
#include "Source/UI/Animation/UIPopupAnimation.h"
#include "Source/UI/UISprite.h"
#include <array>
#include <optional>


namespace app
{
	namespace ui
	{
		class MenuInput;


		/**
		 * @brief どこから遊ぶかを選ぶ画面
		 */
		class StartStageMenu
		{
		public:
			/** 操作の結果 */
			enum class Result
			{
				None,			//!< 何もない
				NewGame,		//!< 最初から
				Continue,		//!< 続きから
				StageSelect,	//!< ステージセレクト画面を開く
				Cancel,			//!< 閉じる
			};

			/** 項目（上から順） */
			enum class Item : int
			{
				NewGame,
				Continue,
				StageSelect,
				Num
			};


		public:
			StartStageMenu();
			~StartStageMenu();
			StartStageMenu(const StartStageMenu&) = delete;
			StartStageMenu& operator=(const StartStageMenu&) = delete;

			/** 画像を読み込む */
			void Init();
			/**
			 * @brief 開く（続きから遊べるなら CONTINUE、遊べないなら NEW GAME を選んだ状態）
			 * @param continueStage 続きのステージ（StageCatalog の表の番号。空なら CONTINUE は灰色で選べない）
			 */
			void Open(const std::optional<int>& continueStage);
			/** 選んでいる項目はそのままで、もう一度開く（ステージセレクトから B で戻ってきたとき） */
			void Reopen();
			/** 閉じる（ウィンドウが小さくなって消える） */
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


		private:
			/** カーソルを動かす（選べない項目は飛ばす。step は +1 で下、-1 で上） */
			void MoveCursor(int step);
			/** その項目を選べるか（CONTINUE は続きの記録があるときだけ） */
			bool IsSelectable(int index) const;
			/** 全部の画像を置き直す（選んでいる項目は黄色で、ゆっくり脈打つ。ウィンドウの拡大率・濃さもかける） */
			void UpdateVisual();
			/** 項目の色（選んでいる・選んでいない・選べない） */
			Vector4 GetItemColor(int index) const;
			/** 項目の高さ（Y。上から index 番目） */
			float GetRowY(int index) const;


		private:
			static constexpr int ITEM_COUNT = static_cast<int>(Item::Num);

			/** 選んでいる項目 */
			int m_index;
			/** 続きから遊べるか（CONTINUE を選べるか） */
			bool m_canContinue;
			/** 演出用のタイマー（秒。カーソルを動かすと 0 に戻る） */
			float m_timer;
			/** ウィンドウの出し入れ（拡大縮小） */
			UIPopupAnimation m_popup;

			// ---- 画像 ----
			/** ウィンドウ（見出しの帯つき） */
			MenuWindow m_window;
			/** 見出し（START） */
			UISprite m_header;
			/** NEW GAME / CONTINUE / STAGE SELECT の文字 */
			std::array<UISprite, ITEM_COUNT> m_items;
			/** CONTINUE の右に出す続きのステージ（「STAGE 5」） */
			StageNameLabel m_continueStage;
			/** 選んでいる項目の印（ひし形） */
			UISprite m_cursor;
			/** 操作の案内（Ⓐ OK　Ⓑ BACK） */
			MenuGuide m_guide;
		};
	}
}
