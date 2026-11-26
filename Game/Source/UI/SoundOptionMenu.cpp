/**
 * @file SoundOptionMenu.cpp
 * @brief サウンドオプション画面
 */
#include "stdafx.h"
#include "SoundOptionMenu.h"
#include <cmath>
#include "Source/Core/Math/MathUtil.h"
#include "Source/Sound/SoundManager.h"
#include "Source/UI/MenuInput.h"


namespace
{
	// ---- 画像（パスと元の大きさ） ----
	const char* const TITLE_PATH = "Assets/sprite/pause/item_sound.dds";
	const char* const BLOCK_PATH = "Assets/sprite/pause/block.dds";
	const char* const LABEL_MASTER_PATH = "Assets/sprite/pause/label_master.dds";
	const char* const LABEL_BGM_PATH = "Assets/sprite/pause/label_bgm.dds";
	const char* const LABEL_SE_PATH = "Assets/sprite/pause/label_se.dds";
	/**
	 * 文字の画像は、文字の大きさと縦の範囲がそろえてあり、高さは全部 TEXT_PIXEL_H（Tools/make_menu_sprites.py が出す大きさ）。
	 * 同じ高さで出せば、どの文字も同じ大きさになる（大文字の高さは、出す高さの約 8 割）
	 */
	constexpr float TEXT_PIXEL_H = 306.0f;
	constexpr float TITLE_PIXEL_W = 1152.0f;
	constexpr float LABEL_MASTER_PIXEL_W = 1149.0f;
	constexpr float LABEL_BGM_PIXEL_W = 764.0f;
	constexpr float LABEL_SE_PIXEL_W = 476.0f;

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	/**
	 * ウィンドウ（真ん中・幅と高さ・見出しの帯の高さ）
	 * 中身（ひし形の印から目盛りの右端まで）が左右の余白 55 くらいで収まる大きさ
	 */
	const app::ui::MenuWindowShape WINDOW_SHAPE = { Vector2(0.0f, 60.0f), Vector2(1060.0f, 640.0f), 130.0f };
	/** 見出し（帯の真ん中に置く） */
	constexpr float TITLE_HEIGHT = 90.0f;
	/** 行の間（帯の下の真ん中に、この間で並べる） */
	constexpr float ROW_PITCH = 140.0f;
	/** 文字の高さ（大文字の高さが、目盛りの大きさとだいたい同じになる） */
	constexpr float LABEL_HEIGHT = 56.0f;
	/** 文字の右端のX（右寄せにする。全体がウィンドウの真ん中に来るように、前より 38 右へ） */
	constexpr float LABEL_RIGHT_X = -192.0f;
	/** 目盛りの1つ目の中心のX と間隔・大きさ */
	constexpr float BLOCK_START_X = -112.0f;
	constexpr float BLOCK_PITCH = 62.0f;
	constexpr float BLOCK_SIZE = 46.0f;
	constexpr float CURSOR_SIZE = 34.0f;
	constexpr float CURSOR_GAP = 50.0f;

	/** 音量を1回で変える量 */
	constexpr float VOLUME_STEP = 0.1f;
}


namespace app
{
	namespace ui
	{
		void SoundOptionMenu::Init()
		{
			m_window.Init(WINDOW_SHAPE);
			// 文字には縁を付ける（目盛りと印はウィンドウの中なので縁なし）
			m_title.InitWithOutline(TITLE_PATH, TITLE_PIXEL_W, TEXT_PIXEL_H, TITLE_HEIGHT);
			m_labels[static_cast<int>(Item::Master)].InitWithOutline(LABEL_MASTER_PATH, LABEL_MASTER_PIXEL_W, TEXT_PIXEL_H, LABEL_HEIGHT);
			m_labels[static_cast<int>(Item::BGM)].InitWithOutline(LABEL_BGM_PATH, LABEL_BGM_PIXEL_W, TEXT_PIXEL_H, LABEL_HEIGHT);
			m_labels[static_cast<int>(Item::SE)].InitWithOutline(LABEL_SE_PATH, LABEL_SE_PIXEL_W, TEXT_PIXEL_H, LABEL_HEIGHT);
			for (auto& row : m_volumeBlocks)
			{
				for (UISprite& block : row)
				{
					block.Init(BLOCK_PATH, 64.0f, 64.0f, BLOCK_SIZE);
				}
			}
			m_cursor.Init(BLOCK_PATH, 64.0f, 64.0f, CURSOR_SIZE);
			// 案内：Ⓑ BACK（右下）
			m_guide.Init(MenuGuideButtons::Back);

			// 動かない画像はここで置いておく（見出しはウィンドウの帯の真ん中）
			m_title.Place(WINDOW_SHAPE.center.x, WINDOW_SHAPE.GetHeaderCenterY(), 1.0f, menu_color::Selected);
		}


		void SoundOptionMenu::Open()
		{
			m_index = static_cast<int>(Item::Master);
			UpdateVisual();
		}


		bool SoundOptionMenu::Update(const MenuInput& input)
		{
			if (input.IsUp())
			{
				m_index = (m_index + ITEM_COUNT - 1) % ITEM_COUNT;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}
			else if (input.IsDown())
			{
				m_index = (m_index + 1) % ITEM_COUNT;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}

			// 左右で音量を変える（10段階。丸めて誤差がたまらないようにする）
			if (input.IsLeft() || input.IsRight())
			{
				const Item item = GetSelectedItem();
				const float volume = GetVolume(item);
				const float direction = input.IsRight() ? 1.0f : -1.0f;
				const float next = math::Clamp01(std::round(volume / VOLUME_STEP + direction) * VOLUME_STEP);
				if (next != volume)
				{
					SetVolume(item, next);
					// SE の音量はすぐ聞き比べられるように、変えたあとの音量で鳴らす
					SoundManager::Get().PlaySE(enSoundKind_CursorMove);
				}
			}

			if (input.IsCancel())
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonBack);
				return true;
			}
			UpdateVisual();
			return false;
		}


		void SoundOptionMenu::UpdateVisual()
		{
			for (int row = 0; row < ITEM_COUNT; ++row)
			{
				const bool isSelected = row == m_index;
				UISprite& label = m_labels[row];
				// 文字は右寄せ
				label.Place(LABEL_RIGHT_X - label.width * 0.5f, GetRowY(row), 1.0f, isSelected ? menu_color::Selected : menu_color::Normal);

				// 目盛り：音量の分だけ光らせる
				const int litCount = static_cast<int>(std::round(GetVolume(static_cast<Item>(row)) * BLOCK_COUNT));
				for (int i = 0; i < BLOCK_COUNT; ++i)
				{
					const bool isLit = i < litCount;
					const Vector4& color = isLit ? (isSelected ? menu_color::Selected : menu_color::White) : menu_color::BlockOff;
					m_volumeBlocks[row][i].Place(BLOCK_START_X + BLOCK_PITCH * static_cast<float>(i), GetRowY(row), 1.0f, color);
				}
			}
			// 選んでいる行の左にひし形の印
			const UISprite& label = m_labels[m_index];
			m_cursor.Place(LABEL_RIGHT_X - label.width - CURSOR_GAP, GetRowY(m_index), 1.0f, menu_color::Selected, 45.0f);
		}


		float SoundOptionMenu::GetRowY(const int row)
		{
			// 帯の下の真ん中を中心に、ROW_PITCH ずつ並べる
			const float centerOffset = (static_cast<float>(ITEM_COUNT) - 1.0f) * 0.5f - static_cast<float>(row);
			return WINDOW_SHAPE.GetBodyCenterY() + ROW_PITCH * centerOffset;
		}


		void SoundOptionMenu::Render(RenderContext& rc)
		{
			// 描いた順に重なる（ウィンドウがいちばん下）
			m_window.Draw(rc);
			m_title.Draw(rc);
			for (int row = 0; row < ITEM_COUNT; ++row)
			{
				m_labels[row].Draw(rc);
				for (UISprite& block : m_volumeBlocks[row])
				{
					block.Draw(rc);
				}
			}
			m_cursor.Draw(rc);
			m_guide.Draw(rc);
		}


		float SoundOptionMenu::GetVolume(const Item item)
		{
			const SoundManager& sound = SoundManager::Get();
			switch (item)
			{
			case Item::Master:	return sound.GetMasterVolume();
			case Item::BGM:		return sound.GetBGMVolume();
			case Item::SE:		return sound.GetSEVolume();
			default:			return 0.0f;
			}
		}


		void SoundOptionMenu::SetVolume(const Item item, const float volume)
		{
			SoundManager& sound = SoundManager::Get();
			switch (item)
			{
			case Item::Master:	sound.SetMasterVolume(volume); break;
			case Item::BGM:		sound.SetBGMVolume(volume); break;
			case Item::SE:		sound.SetSEVolume(volume); break;
			default: break;
			}
		}
	}
}
