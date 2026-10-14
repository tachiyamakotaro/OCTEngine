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
	const char* const BACKGROUND_PATH = "Assets/sprite/fade.dds";
	const char* const TITLE_PATH = "Assets/sprite/pause/item_sound.dds";
	const char* const BLOCK_PATH = "Assets/sprite/pause/block.dds";
	const char* const LABEL_MASTER_PATH = "Assets/sprite/pause/label_master.dds";
	const char* const LABEL_BGM_PATH = "Assets/sprite/pause/label_bgm.dds";
	const char* const LABEL_SE_PATH = "Assets/sprite/pause/label_se.dds";
	const char* const LABEL_BACK_PATH = "Assets/sprite/pause/label_back.dds";
	const char* const BUTTON_B_PATH = "Assets/sprite/pause/button_b.dds";

	/** 背景の濃さ */
	constexpr float BACKGROUND_ALPHA = 0.65f;

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	constexpr float TITLE_Y = 300.0f;
	constexpr float TITLE_HEIGHT = 120.0f;
	constexpr float ROW_Y[] = { 110.0f, -30.0f, -170.0f };
	constexpr float LABEL_HEIGHT = 70.0f;
	/** 文字の右端のX（右寄せにする） */
	constexpr float LABEL_RIGHT_X = -230.0f;
	/** 目盛りの1つ目の中心のX と間隔・大きさ */
	constexpr float BLOCK_START_X = -150.0f;
	constexpr float BLOCK_PITCH = 62.0f;
	constexpr float BLOCK_SIZE = 46.0f;
	constexpr float CURSOR_SIZE = 34.0f;
	constexpr float CURSOR_GAP = 50.0f;

	constexpr float GUIDE_X = 640.0f;
	constexpr float GUIDE_Y = -430.0f;
	constexpr float GUIDE_ICON_SIZE = 80.0f;
	constexpr float GUIDE_LABEL_HEIGHT = 50.0f;

	/** 音量を1回で変える量 */
	constexpr float VOLUME_STEP = 0.1f;
}


namespace app
{
	namespace ui
	{
		void SoundOptionMenu::Init()
		{
			m_background.InitFullScreen(BACKGROUND_PATH);
			m_title.Init(TITLE_PATH, 1155.0f, 268.0f, TITLE_HEIGHT);
			m_labels[static_cast<int>(Item::Master)].Init(LABEL_MASTER_PATH, 442.0f, 96.0f, LABEL_HEIGHT);
			m_labels[static_cast<int>(Item::BGM)].Init(LABEL_BGM_PATH, 252.0f, 96.0f, LABEL_HEIGHT);
			m_labels[static_cast<int>(Item::SE)].Init(LABEL_SE_PATH, 147.0f, 96.0f, LABEL_HEIGHT);
			for (auto& row : m_volumeBlocks)
			{
				for (UISprite& block : row)
				{
					block.Init(BLOCK_PATH, 64.0f, 64.0f, BLOCK_SIZE);
				}
			}
			m_cursor.Init(BLOCK_PATH, 64.0f, 64.0f, CURSOR_SIZE);
			m_buttonB.Init(BUTTON_B_PATH, 128.0f, 128.0f, GUIDE_ICON_SIZE);
			m_labelBack.Init(LABEL_BACK_PATH, 304.0f, 96.0f, GUIDE_LABEL_HEIGHT);

			// 動かない画像はここで置いておく
			m_background.Place(0.0f, 0.0f, 1.0f, Vector4(1.0f, 1.0f, 1.0f, BACKGROUND_ALPHA));
			m_title.Place(0.0f, TITLE_Y, 1.0f, menu_color::Selected);
			// 案内：B BACK（右下）
			m_buttonB.Place(GUIDE_X, GUIDE_Y, 1.0f, menu_color::White);
			m_labelBack.Place(GUIDE_X + GUIDE_ICON_SIZE * 0.5f + 10.0f + m_labelBack.width * 0.5f, GUIDE_Y, 1.0f, menu_color::White);
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
				label.Place(LABEL_RIGHT_X - label.width * 0.5f, ROW_Y[row], 1.0f, isSelected ? menu_color::Selected : menu_color::Normal);

				// 目盛り：音量の分だけ光らせる
				const int litCount = static_cast<int>(std::round(GetVolume(static_cast<Item>(row)) * BLOCK_COUNT));
				for (int i = 0; i < BLOCK_COUNT; ++i)
				{
					const bool isLit = i < litCount;
					const Vector4& color = isLit ? (isSelected ? menu_color::Selected : menu_color::White) : menu_color::BlockOff;
					m_volumeBlocks[row][i].Place(BLOCK_START_X + BLOCK_PITCH * static_cast<float>(i), ROW_Y[row], 1.0f, color);
				}
			}
			// 選んでいる行の左にひし形の印
			const UISprite& label = m_labels[m_index];
			m_cursor.Place(LABEL_RIGHT_X - label.width - CURSOR_GAP, ROW_Y[m_index], 1.0f, menu_color::Selected, 45.0f);
		}


		void SoundOptionMenu::Render(RenderContext& rc)
		{
			// 描いた順に重なる（背景がいちばん下）
			m_background.Draw(rc);
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
			m_buttonB.Draw(rc);
			m_labelBack.Draw(rc);
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
