/**
 * @file TitleMenu.h
 * @brief タイトルメニュー（CONTINUE / START / SELECT / SOUND / QUIT を選ぶ）
 * @details 操作
 *          ・タイトルメニュー
 *              左右（上下でも可。十字キー・左スティック）… CONTINUE / START / SELECT / SOUND / QUIT を選ぶ
 *              A … 決定（CONTINUE → 続きから、START → 始めから、SELECT → ステージセレクト、SOUND → サウンドオプション、QUIT → ゲームを終わる）
 *          ・CONTINUE は続きの記録がないときは灰色で、選べない（カーソルが飛ばす）
 *          ・ステージセレクト画面（ui::StageSelectMenu。クリアしたかに関係なく、どのステージでも選べる）
 *              A … そのステージから遊ぶ（タイトルメニューに戻って SELECT が点滅する）
 *              B … タイトルメニューに戻る
 *          ・サウンドオプション画面（ui::SoundOptionMenu。ポーズ画面と共通）
 *              B … タイトルメニューに戻る
 *          ボタンの画像は Assets/sprite/title/。
 *          白い文字を SetMulColor で色付けしている。
 *          ・タイトルのロゴ（logo.dds）とボタンの後ろの暗い帯（menu_shade.dds）もこのクラスで出す
 *              ロゴ … 開いてから少しあとに、少し大きい所から縮みながら出てきて、そのあとゆっくり上下にゆれる
 *              帯   … 下から上へ薄くなる黒。3D の地面の上でもボタンの文字を読めるようにする
 *            （画像は Tools/title_image/make_title_scene_sprites.py で作った）
 */
#pragma once
#include <array>
#include "Source/UI/MenuInput.h"
#include "Source/UI/SoundOptionMenu.h"
#include "Source/UI/StageSelect/StageSelectMenu.h"
#include "Source/UI/UISprite.h"


namespace app
{
	namespace ui
	{
		/**
		 * @brief タイトルメニュー
		 */
		class TitleMenu
		{
		public:
			/** 操作の結果 */
			enum class Result
			{
				None,		//!< 何もない
				ContinueGame,	//!< 続きから遊ぶ
				StartGame,	//!< ゲームを始める（始めから）
				SelectStage,	//!< ステージセレクトで選んだステージから遊ぶ（GetSelectedStage で選んだステージがわかる）
				QuitGame,	//!< ゲームを終わる
			};

			/** 項目（左から順） */
			enum class Item : int
			{
				Continue,
				Start,
				StageSelect,
				Sound,
				Quit,
				Num
			};

			/** 今の画面 */
			enum class Page
			{
				Main,
				StageSelect,
				Sound,
			};


		public:
			TitleMenu() = default;
			TitleMenu(const TitleMenu&) = delete;
			TitleMenu& operator=(const TitleMenu&) = delete;

			/** 画像を読み込む */
			void Init();
			/**
			 * @brief 開く（続きから遊べるなら CONTINUE、遊べないなら START を選んだ状態）
			 * @param canContinue 続きから遊べるか（false なら CONTINUE は灰色で選べない）
			 */
			void Open(bool canContinue);
			/** 入力を見て操作する（毎フレーム呼ぶ。ContinueGame / StartGame / SelectStage / QuitGame を返したあとは入力を受け付けない） */
			Result Update();
			/** 演出だけ動かす（シーンの PauseUpdate から呼ぶ。フェード中も点滅させるため） */
			void Animate();
			/** 描く */
			void Render(RenderContext& rc);

			inline Page GetPage() const { return m_page; }
			inline Item GetSelectedItem() const { return static_cast<Item>(m_index); }
			/** ステージセレクトで選んだステージ（StageCatalog の表の番号。Update が SelectStage を返したときに使う） */
			inline int GetSelectedStage() const { return m_stageSelect.GetSelectedStage(); }


		private:
			Result UpdateMain();
			/** ステージセレクト画面の操作（決めたらタイトルメニューに戻して SelectStage を返す） */
			Result UpdateStageSelect();
			/** 項目の見た目（選んでいる項目は色を変えて、少し大きくして、ゆっくり脈打たせる）を更新する */
			void UpdateMainVisual();
			/** カーソルを動かす（選べない項目は飛ばす。step は +1 で右、-1 で左） */
			void MoveCursor(int step);
			/** その項目を選べるか（CONTINUE は続きの記録があるときだけ） */
			bool IsSelectable(int index) const;
			/** ボタンを横に並べる（ボタンの幅から、すき間がそろって全体が画面の真ん中に来る位置を決める） */
			void LayoutItems();
			/** ロゴの見た目（出てくる・ゆれる）を更新する */
			void UpdateLogoVisual();


		private:
			static constexpr int ITEM_COUNT = static_cast<int>(Item::Num);

			/** 今の画面 */
			Page m_page = Page::Main;
			/** 選んでいる項目 */
			int m_index = 0;
			/** 決定したか（決定したあとは操作を受け付けず、選んだ項目を点滅させる） */
			bool m_isDecided = false;
			/** 続きから遊べるか（CONTINUE を選べるか） */
			bool m_canContinue = false;
			/** 演出用のタイマー（秒。カーソルを動かしたり決定したりすると 0 に戻る） */
			float m_timer = 0.0f;
			/** 開いてからの時間（秒。ロゴが出てくる・ゆれるのに使う。0 に戻さない） */
			float m_openTime = 0.0f;
			/** カーソル操作 */
			MenuInput m_input;
			/** サウンドオプション画面 */
			SoundOptionMenu m_soundOption;
			/** ステージセレクト画面 */
			StageSelectMenu m_stageSelect;

			// ---- 画像 ----
			/** CONTINUE / START / SELECT / SOUND / QUIT のボタン */
			std::array<UISprite, ITEM_COUNT> m_items;
			/** ボタンの中心の X（LayoutItems で決める） */
			std::array<float, ITEM_COUNT> m_itemX = {};
			/** 選んでいる項目の印（ひし形） */
			UISprite m_cursor;
			/** タイトルのロゴ */
			UISprite m_logo;
			/** ボタンの後ろの暗い帯 */
			UISprite m_buttonShade;
		};
	}
}
