/**
 * @file PauseMenu.h
 * @brief ポーズ画面（SOUND / TITLE を選ぶ）とサウンドオプション画面
 * @details 操作
 *          ・ポーズ画面
 *              上下（十字キー・左スティック）… SOUND / TITLE を選ぶ
 *              A … 決定（SOUND → サウンドオプション、TITLE → タイトルへ戻る）
 *              B / -（Select）… ポーズ解除
 *          ・サウンドオプション画面
 *              上下 … MASTER / BGM / SE を選ぶ
 *              左右 … 音量を 10% ずつ変える（すぐに SoundManager に反映）
 *              B … ポーズ画面に戻る、-（Select）… ポーズ解除
 *          見た目は全部画像（SpriteRender）で作っている。背景はフェードの黒い画像を半透明にして使う。
 *          画像は Assets/sprite/pause/（Tools/make_pause_sprites.py で作ったもの）。
 */
#pragma once
#include <array>
#include <memory>


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
			};

			/** ポーズ画面の項目（上から順） */
			enum class MainItem : int
			{
				Sound,
				Title,
				Num
			};

			/** サウンドオプションの項目（上から順） */
			enum class SoundItem : int
			{
				Master,
				BGM,
				SE,
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
			inline SoundItem GetSelectedSoundItem() const { return static_cast<SoundItem>(m_soundIndex); }


		private:
			/** 画像1枚と、その元の大きさ */
			struct Sprite2D
			{
				SpriteRender render;
				float width = 0.0f;
				float height = 0.0f;
			};


		private:
			Result UpdateMain();
			void UpdateSound();
			/** 項目の見た目（選んでいる項目は黄色で少し大きく）を更新する */
			void UpdateMainVisual();
			void UpdateSoundVisual();

			/** 画像を読み込む（表示する高さに合わせて、元の縦横比のまま大きさを決める） */
			static void InitSprite(Sprite2D& sprite, const char* path, float pixelWidth, float pixelHeight, float displayHeight);
			/** 置く */
			static void Place(Sprite2D& sprite, float x, float y, float scale, const Vector4& color, float rotationDeg = 0.0f);

			// ---- 入力（十字キーとスティックのどちらでも。スティックは倒した瞬間だけ） ----
			bool IsUpTrigger();
			bool IsDownTrigger();
			bool IsLeftTrigger();
			bool IsRightTrigger();
			/** スティックの入力を押した瞬間に変える */
			void UpdateStickTrigger();

			/** 音量を取得・設定する */
			static float GetVolume(SoundItem item);
			static void SetVolume(SoundItem item, float volume);


		private:
			/** 今の画面 */
			Page m_page;
			/** 選んでいる項目 */
			int m_mainIndex;
			int m_soundIndex;

			// ---- 画像 ----
			/** 背景（フェードの黒い画像を半透明にして使う） */
			Sprite2D m_background;
			/** PAUSE の文字 */
			Sprite2D m_pauseTitle;
			/** SOUND / TITLE の項目 */
			std::array<Sprite2D, static_cast<size_t>(MainItem::Num)> m_mainItems;
			/** 選んでいる項目の印（ひし形） */
			Sprite2D m_cursor;
			/** サウンドオプションの見出し（SOUND の文字） */
			Sprite2D m_soundTitle;
			/** MASTER / BGM / SE の文字 */
			std::array<Sprite2D, static_cast<size_t>(SoundItem::Num)> m_soundLabels;
			/** 音量の目盛り（項目ごとに10個） */
			std::array<std::array<Sprite2D, 10>, static_cast<size_t>(SoundItem::Num)> m_volumeBlocks;
			/** 操作の案内（A OK / B BACK） */
			Sprite2D m_buttonA;
			Sprite2D m_buttonB;
			Sprite2D m_labelOk;
			Sprite2D m_labelBack;

			// ---- スティック ----
			/** スティックを上下・左右に倒しているか（倒した瞬間を見るため） */
			bool m_isStickUp;
			bool m_isStickDown;
			bool m_isStickLeft;
			bool m_isStickRight;
			/** 今フレーム倒した瞬間か */
			bool m_stickUpTrigger;
			bool m_stickDownTrigger;
			bool m_stickLeftTrigger;
			bool m_stickRightTrigger;
		};
	}
}
