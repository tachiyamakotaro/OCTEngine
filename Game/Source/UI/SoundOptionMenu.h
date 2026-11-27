/**
 * @file SoundOptionMenu.h
 * @brief サウンドオプション画面（MASTER / BGM / SE の音量を変える）
 * @details タイトルメニューとポーズ画面の両方から開く。
 *          操作
 *              上下 … MASTER / BGM / SE を選ぶ
 *              左右 … 音量を 10% ずつ変える（すぐに SoundManager に反映）
 *              B    … 閉じる（Update が true を返すので、呼んだ側で前の画面に戻す）
 *          見た目：見出しの帯のあるウィンドウ（MenuWindow）に入れて、文字には暗い縁を付ける。
 *          ウィンドウは拡大縮小で出し入れする（UIPopupAnimation）。Open で大きくなって出て、Close で小さくなって消える。
 *          閉じている途中も描くので、呼んだ側は毎フレーム Animate と Render を呼ぶ（見えていないときは何もしない）。
 *          右下の操作の案内（Ⓑ BACK）も小さなウィンドウ（MenuGuide）。
 *          後ろの画面（ぼかし・暗さ）は呼んだ側が描く（タイトルは MenuBackdrop、ポーズ画面は半透明の黒）。
 */
#pragma once
#include <array>
#include "Source/UI/MenuGuide.h"
#include "Source/UI/MenuWindow.h"
#include "Source/UI/Animation/UIPopupAnimation.h"
#include "Source/UI/UISprite.h"


namespace app
{
	namespace ui
	{
		class MenuInput;


		/**
		 * @brief サウンドオプション画面
		 */
		class SoundOptionMenu
		{
		public:
			/** 項目（上から順） */
			enum class Item : int
			{
				Master,
				BGM,
				SE,
				Num
			};


		public:
			SoundOptionMenu() = default;
			SoundOptionMenu(const SoundOptionMenu&) = delete;
			SoundOptionMenu& operator=(const SoundOptionMenu&) = delete;

			/** 画像を読み込む */
			void Init();
			/** 開く（MASTER を選んだ状態にして、ウィンドウが大きくなって出てくる） */
			void Open();
			/** 閉じる（ウィンドウが小さくなって消える） */
			void Close();
			/** すぐに消す（アニメーションしない。ポーズを解いたときなど） */
			void Hide();
			/**
			 * @brief 入力を見て操作する（開いている間、毎フレーム呼ぶ）
			 * @param input 呼んだ側のカーソル操作（Update() 済みのもの）
			 * @return 閉じたいなら true（B が押された。Close は呼んだ側がする）
			 */
			bool Update(const MenuInput& input);
			/** 開閉のアニメーションを進めて、置き直す（開いていないときも毎フレーム呼ぶ） */
			void Animate();
			/** 描く（見えていないときは何もしない） */
			void Render(RenderContext& rc);
			/** 見えているか（開いている・開いている途中・閉じている途中） */
			bool IsVisible() const { return m_popup.IsVisible(); }

			inline Item GetSelectedItem() const { return static_cast<Item>(m_index); }


		private:
			/** 全部の画像を置き直す（選んでいる行は黄色。目盛り。ウィンドウの拡大率・濃さもかける） */
			void UpdateVisual();
			/** 行の高さ（Y。上から row 番目） */
			static float GetRowY(int row);

			/** 音量を取得・設定する */
			static float GetVolume(Item item);
			static void SetVolume(Item item, float volume);


		private:
			static constexpr int ITEM_COUNT = static_cast<int>(Item::Num);
			static constexpr int BLOCK_COUNT = 10;

			/** 選んでいる項目 */
			int m_index = 0;
			/** ウィンドウの出し入れ（拡大縮小） */
			UIPopupAnimation m_popup;

			// ---- 画像 ----
			/** ウィンドウ（見出しの帯つき） */
			MenuWindow m_window;
			/** 見出し（SOUND の文字） */
			UISprite m_title;
			/** MASTER / BGM / SE の文字 */
			std::array<UISprite, ITEM_COUNT> m_labels;
			/** 音量の目盛り（項目ごとに10個） */
			std::array<std::array<UISprite, BLOCK_COUNT>, ITEM_COUNT> m_volumeBlocks;
			/** 選んでいる行の印（ひし形） */
			UISprite m_cursor;
			/** 操作の案内（Ⓑ BACK） */
			MenuGuide m_guide;
		};
	}
}
