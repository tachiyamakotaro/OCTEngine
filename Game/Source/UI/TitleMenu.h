/**
 * @file TitleMenu.h
 * @brief タイトルメニュー（START / SOUND / QUIT を選ぶ）
 * @details 操作
 *          ・タイトルメニュー
 *              左右（上下でも可。十字キー・左スティック）… START / SOUND / QUIT を選ぶ
 *              A … 決定（START → インゲームへ、SOUND → サウンドオプション、QUIT → ゲームを終わる）
 *          ・サウンドオプション画面（ui::SoundOptionMenu。ポーズ画面と共通）
 *              B … タイトルメニューに戻る
 *          ボタンの画像は Assets/sprite/title/（Tools/make_text_sprite.py で、白い文字・透明な背景にしたもの）。
 *          白い文字を SetMulColor で色付けしている。
 */
#pragma once
#include <array>
#include "Source/UI/MenuInput.h"
#include "Source/UI/SoundOptionMenu.h"
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
				StartGame,	//!< ゲームを始める
				QuitGame,	//!< ゲームを終わる
			};

			/** 項目（左から順） */
			enum class Item : int
			{
				Start,
				Sound,
				Quit,
				Num
			};

			/** 今の画面 */
			enum class Page
			{
				Main,
				Sound,
			};


		public:
			TitleMenu() = default;
			TitleMenu(const TitleMenu&) = delete;
			TitleMenu& operator=(const TitleMenu&) = delete;

			/** 画像を読み込む */
			void Init();
			/** 開く（START を選んだ状態） */
			void Open();
			/** 入力を見て操作する（毎フレーム呼ぶ。StartGame / QuitGame を返したあとは入力を受け付けない） */
			Result Update();
			/** 演出だけ動かす（シーンの PauseUpdate から呼ぶ。フェード中も点滅させるため） */
			void Animate();
			/** 描く */
			void Render(RenderContext& rc);

			inline Page GetPage() const { return m_page; }
			inline Item GetSelectedItem() const { return static_cast<Item>(m_index); }


		private:
			Result UpdateMain();
			/** 項目の見た目（選んでいる項目は色を変えて、少し大きくして、ゆっくり脈打たせる）を更新する */
			void UpdateMainVisual();


		private:
			static constexpr int ITEM_COUNT = static_cast<int>(Item::Num);

			/** 今の画面 */
			Page m_page = Page::Main;
			/** 選んでいる項目 */
			int m_index = 0;
			/** 決定したか（決定したあとは操作を受け付けず、選んだ項目を点滅させる） */
			bool m_isDecided = false;
			/** 演出用のタイマー（秒） */
			float m_timer = 0.0f;
			/** カーソル操作 */
			MenuInput m_input;
			/** サウンドオプション画面 */
			SoundOptionMenu m_soundOption;

			// ---- 画像 ----
			/** START / SOUND / QUIT のボタン */
			std::array<UISprite, ITEM_COUNT> m_items;
			/** 選んでいる項目の印（ひし形） */
			UISprite m_cursor;
		};
	}
}
