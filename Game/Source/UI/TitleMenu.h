/**
 * @file TitleMenu.h
 * @brief タイトルメニュー（START / SOUND / QUIT を選ぶ）
 * @details 操作
 *          ・タイトルメニュー
 *              左右（上下でも可。十字キー・左スティック）… START / SOUND / QUIT を選ぶ
 *              A … 決定（START → どこから遊ぶかを選ぶ、SOUND → サウンドオプション、QUIT → ゲームを終わる）
 *          ・START の中（どこから遊ぶかを選ぶ画面。ui::StartStageMenu）
 *              NEW GAME     … 最初のステージから（始めから）
 *              CONTINUE     … 続きのステージから（続きの記録がないときは灰色で、選べない）
 *              STAGE SELECT … ステージセレクト画面を開く
 *              B … タイトルメニューに戻る
 *          ・ステージセレクト画面（ui::StageSelectMenu。クリアしたかに関係なく、どのステージでも選べる）
 *              A … そのステージから遊ぶ（NEW GAME と同じく、新しいプレイになる）
 *              B … START の中に戻る
 *          ・遊ぶステージを決めると、タイトルメニューに戻って START が点滅する
 *          ・サウンドオプション画面（ui::SoundOptionMenu。ポーズ画面と共通）
 *              B … タイトルメニューに戻る
 *          ボタンの画像は Assets/sprite/title/。
 *          白い文字を SetMulColor で色付けしている。
 *          ・タイトルのロゴ（logo.dds）とボタンの後ろの暗い帯（menu_shade.dds）もこのクラスで出す
 *              ロゴ … 開いてから少しあとに、少し大きい所から縮みながら出てきて、そのあとゆっくり上下にゆれる
 *              帯   … 下から上へ薄くなる黒。3D の地面の上でもボタンの文字を読めるようにする
 *            （画像は Tools/title_image/make_title_scene_sprites.py で作った）
 *          ・ボタンの文字とひし形の印には暗い縁を付ける（UISprite::InitWithOutline）
 *          ・ほかの画面（START の中・ステージセレクト・サウンドオプション）はウィンドウで出す。
 *            開いている間は、後ろの 3D の絵をぼかして少し暗くし（MenuBackdrop）、ロゴとボタンを薄くして消す
 *            ウィンドウは拡大縮小で出し入れする。画面を移るときは、前の画面が小さくなって消えるのと同時に、次の画面が大きくなって出てくる
 */
#pragma once
#include <array>
#include "Source/UI/MenuBackdrop.h"
#include "Source/UI/MenuInput.h"
#include "Source/UI/SoundOptionMenu.h"
#include "Source/UI/StageSelect/StageSelectMenu.h"
#include "Source/UI/StageSelect/StartStageMenu.h"
#include "Source/UI/UISprite.h"
#include <optional>


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
				ContinueGame,	//!< 続きから遊ぶ（遊ぶステージは GetStartStage）
				StartGame,	//!< 新しく遊ぶ（NEW GAME・ステージセレクト。遊ぶステージは GetStartStage）
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
				StartStage,		//!< START の中（どこから遊ぶかを選ぶ）
				StageSelect,	//!< ステージセレクト
				Sound,
			};


		public:
			TitleMenu() = default;
			TitleMenu(const TitleMenu&) = delete;
			TitleMenu& operator=(const TitleMenu&) = delete;

			/** 画像を読み込む */
			void Init();
			/**
			 * @brief 開く（START を選んだ状態）
			 * @param continueStage 続きから遊ぶステージ（save::LoadProgress の結果。空なら START の中の CONTINUE は灰色で選べない）
			 */
			void Open(const std::optional<int>& continueStage);
			/** 入力を見て操作する（毎フレーム呼ぶ。ContinueGame / StartGame / QuitGame を返したあとは入力を受け付けない） */
			Result Update();
			/** 演出だけ動かす（シーンの PauseUpdate から呼ぶ。フェード中も点滅させるため） */
			void Animate();
			/** 描く */
			void Render(RenderContext& rc);

			inline Page GetPage() const { return m_page; }
			inline Item GetSelectedItem() const { return static_cast<Item>(m_index); }
			/** 遊ぶステージ（StageCatalog の表の番号。Update が ContinueGame / StartGame を返したときに使う） */
			inline int GetStartStage() const { return m_startStage; }


		private:
			Result UpdateMain();
			/** START の中（どこから遊ぶかを選ぶ画面）の操作 */
			Result UpdateStartStage();
			/** ステージセレクト画面の操作（B では START の中に戻る） */
			Result UpdateStageSelect();
			/**
			 * @brief 遊ぶステージを決める（タイトルメニューに戻って、START を点滅させる）
			 * @param stageIndex 遊ぶステージ（StageCatalog の表の番号）
			 * @param result 返す結果（ContinueGame / StartGame）
			 * @return result をそのまま返す
			 */
			Result DecideStartStage(int stageIndex, Result result);
			/**
			 * @brief 項目の見た目（選んでいる項目は色を変えて、少し大きくして、ゆっくり脈打たせる）を更新する
			 * @details ほかの画面のウィンドウを開いている間は、後ろの画面の強さに合わせて薄くする
			 */
			void UpdateMainVisual();
			/** カーソルを動かす（step は +1 で右、-1 で左。端の次は反対の端） */
			void MoveCursor(int step);
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
			/** 続きから遊ぶステージ（空なら START の中の CONTINUE を選べない） */
			std::optional<int> m_continueStage;
			/** 遊ぶステージ（決めたときに入れる） */
			int m_startStage = 0;
			/** 演出用のタイマー（秒。カーソルを動かしたり決定したりすると 0 に戻る） */
			float m_timer = 0.0f;
			/** 開いてからの時間（秒。ロゴが出てくる・ゆれるのに使う。0 に戻さない） */
			float m_openTime = 0.0f;
			/** カーソル操作 */
			MenuInput m_input;
			/** サウンドオプション画面 */
			SoundOptionMenu m_soundOption;
			/** START の中（どこから遊ぶかを選ぶ画面） */
			StartStageMenu m_startStageMenu;
			/** ステージセレクト画面 */
			StageSelectMenu m_stageSelect;
			/** ほかの画面を開いている間の、後ろの画面（3D の絵をぼかして、少し暗くする） */
			MenuBackdrop m_backdrop;

			// ---- 画像 ----
			/** START / SOUND / QUIT のボタン */
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
