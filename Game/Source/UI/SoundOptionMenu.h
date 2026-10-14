/**
 * @file SoundOptionMenu.h
 * @brief サウンドオプション画面（MASTER / BGM / SE の音量を変える）
 * @details タイトルメニューとポーズ画面の両方から開く。
 *          操作
 *              上下 … MASTER / BGM / SE を選ぶ
 *              左右 … 音量を 10% ずつ変える（すぐに SoundManager に反映）
 *              B    … 閉じる（Update が true を返すので、呼んだ側で前の画面に戻す）
 *          背景（半透明の黒）もこのクラスが描くので、呼んだ側は背景を描かなくてよい。
 */
#pragma once
#include <array>
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
			/** 開く（MASTER を選んだ状態にする） */
			void Open();
			/**
			 * @brief 入力を見て操作する（開いている間、毎フレーム呼ぶ）
			 * @param input 呼んだ側のカーソル操作（Update() 済みのもの）
			 * @return 閉じたいなら true（B が押された）
			 */
			bool Update(const MenuInput& input);
			/** 描く（開いている間だけ呼ぶ） */
			void Render(RenderContext& rc);

			inline Item GetSelectedItem() const { return static_cast<Item>(m_index); }


		private:
			/** 項目の見た目（選んでいる行は黄色）と目盛りを更新する */
			void UpdateVisual();

			/** 音量を取得・設定する */
			static float GetVolume(Item item);
			static void SetVolume(Item item, float volume);


		private:
			static constexpr int ITEM_COUNT = static_cast<int>(Item::Num);
			static constexpr int BLOCK_COUNT = 10;

			/** 選んでいる項目 */
			int m_index = 0;

			// ---- 画像 ----
			/** 背景（フェードの黒い画像を半透明にして使う） */
			UISprite m_background;
			/** 見出し（SOUND の文字） */
			UISprite m_title;
			/** MASTER / BGM / SE の文字 */
			std::array<UISprite, ITEM_COUNT> m_labels;
			/** 音量の目盛り（項目ごとに10個） */
			std::array<std::array<UISprite, BLOCK_COUNT>, ITEM_COUNT> m_volumeBlocks;
			/** 選んでいる行の印（ひし形） */
			UISprite m_cursor;
			/** 操作の案内（B BACK） */
			UISprite m_buttonB;
			UISprite m_labelBack;
		};
	}
}
