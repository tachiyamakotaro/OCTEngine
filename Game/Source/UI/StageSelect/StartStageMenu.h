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
 *          見出しはタイトルの START のボタンと同じ画像。背景（半透明の黒）もこのクラスが描く。
 */
#pragma once
#include "Source/UI/StageSelect/StageNameLabel.h"
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
			/**
			 * @brief 入力を見て操作する（開いている間、毎フレーム呼ぶ）
			 * @param input 呼んだ側のカーソル操作（Update() 済みのもの）
			 */
			Result Update(const MenuInput& input);
			/** 描く（開いている間だけ呼ぶ） */
			void Render(RenderContext& rc);


		private:
			/** カーソルを動かす（選べない項目は飛ばす。step は +1 で下、-1 で上） */
			void MoveCursor(int step);
			/** その項目を選べるか（CONTINUE は続きの記録があるときだけ） */
			bool IsSelectable(int index) const;
			/** 項目の見た目（選んでいる項目は黄色で、ゆっくり脈打つ）と印を更新する */
			void UpdateVisual();
			/** 項目の色（選んでいる・選んでいない・選べない） */
			Vector4 GetItemColor(int index) const;
			/** 動かない画像（背景・見出し・案内）を置く */
			void PlaceFixedSprites();


		private:
			static constexpr int ITEM_COUNT = static_cast<int>(Item::Num);

			/** 選んでいる項目 */
			int m_index;
			/** 続きから遊べるか（CONTINUE を選べるか） */
			bool m_canContinue;
			/** 演出用のタイマー（秒。カーソルを動かすと 0 に戻る） */
			float m_timer;

			// ---- 画像 ----
			/** 背景（フェードの黒い画像を半透明にして使う） */
			UISprite m_background;
			/** 見出し（START） */
			UISprite m_header;
			/** NEW GAME / CONTINUE / STAGE SELECT の文字 */
			std::array<UISprite, ITEM_COUNT> m_items;
			/** CONTINUE の右に出す続きのステージ（「STAGE 5」） */
			StageNameLabel m_continueStage;
			/** 選んでいる項目の印（ひし形） */
			UISprite m_cursor;
			/** 操作の案内（A OK / B BACK） */
			UISprite m_buttonA;
			UISprite m_labelOk;
			UISprite m_buttonB;
			UISprite m_labelBack;
		};
	}
}
