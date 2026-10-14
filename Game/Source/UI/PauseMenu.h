/**
 * @file PauseMenu.h
 * @brief ポーズ画面（SOUND / TITLE を選ぶ）
 * @details 操作
 *          ・ポーズ画面
 *              上下（十字キー・左スティック）… SOUND / TITLE を選ぶ
 *              A … 決定（SOUND → サウンドオプション、TITLE → タイトルへ戻る）
 *              B / -（Select）… ポーズ解除
 *          ・サウンドオプション画面（ui::SoundOptionMenu。タイトルメニューと共通）
 *              B … ポーズ画面に戻る、-（Select）… ポーズ解除
 *          見た目は全部画像（UISprite）で作っている。背景はフェードの黒い画像を半透明にして使う。
 *          画像は Assets/sprite/pause/。
 */
#pragma once
#include "Source/UI/MenuInput.h"
#include "Source/UI/SoundOptionMenu.h"
#include "Source/UI/UISprite.h"
#include <array>


namespace app
{
	namespace ui
	{
		/**
		 * @brief ポーズ画面
		 */
		class PauseMenu
		{
		public:
			/** ポーズ画面の操作の結果 */
			enum class Result
			{
				None,		//!< 何もない（ポーズ中のまま）
				Resume,		//!< ポーズ解除
				GoToTitle,	//!< タイトルへ戻る
				QuitGame
			};

			/** ポーズ画面の項目（上から順） */
			enum class MainItem : int
			{
				Sound,
				Title,
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
			PauseMenu();
			~PauseMenu();
			PauseMenu(const PauseMenu&) = delete;
			PauseMenu& operator=(const PauseMenu&) = delete;

			/** 画像を読み込む */
			void Init();
			/** ポーズ画面を開く（最初は SOUND を選んだ状態） */
			void Open();
			/** 入力を見て操作する（ポーズ中に毎フレーム呼ぶ） */
			Result Update();
			/** 描く（ポーズ中だけ呼ぶ） */
			void Render(RenderContext& rc);

			inline Page GetPage() const { return m_page; }
			inline MainItem GetSelectedMainItem() const { return static_cast<MainItem>(m_mainIndex); }
			inline SoundOptionMenu::Item GetSelectedSoundItem() const { return m_soundOption.GetSelectedItem(); }


		private:
			Result UpdateMain();
			/** 項目の見た目（選んでいる項目は黄色で少し大きく）を更新する */
			void UpdateMainVisual();


		private:
			/** 今の画面 */
			Page m_page;
			/** 選んでいる項目 */
			int m_mainIndex;
			/** カーソル操作 */
			MenuInput m_input;
			/** サウンドオプション画面 */
			SoundOptionMenu m_soundOption;

			// ---- 画像 ----
			/** 背景（フェードの黒い画像を半透明にして使う） */
			UISprite m_background;
			/** PAUSE の文字 */
			UISprite m_pauseTitle;
			/** SOUND / TITLE の項目 */
			std::array<UISprite, static_cast<size_t>(MainItem::Num)> m_mainItems;
			/** 選んでいる項目の印（ひし形） */
			UISprite m_cursor;
			/** 操作の案内（A OK / B BACK） */
			UISprite m_buttonA;
			UISprite m_buttonB;
			UISprite m_labelOk;
			UISprite m_labelBack;
		};
	}
}
