/**
 * @file PauseMenu.cpp
 * @brief ポーズ画面とサウンドオプション画面
 */
#include "stdafx.h"
#include "PauseMenu.h"
#include <cmath>
#include "Source/Core/Math/MathUtil.h"
#include "Source/Sound/SoundManager.h"


namespace
{
	// ---- 画像（パスと元の大きさ。Tools/make_pause_sprites.py で作ったもの） ----
	const char* const BACKGROUND_PATH = "Assets/sprite/fade.dds";
	const char* const PAUSE_TITLE_PATH = "Assets/sprite/pause/pause_title.dds";
	const char* const ITEM_SOUND_PATH = "Assets/sprite/pause/item_sound.dds";
	const char* const ITEM_TITLE_PATH = "Assets/sprite/pause/item_title.dds";
	const char* const BLOCK_PATH = "Assets/sprite/pause/block.dds";
	const char* const LABEL_MASTER_PATH = "Assets/sprite/pause/label_master.dds";
	const char* const LABEL_BGM_PATH = "Assets/sprite/pause/label_bgm.dds";
	const char* const LABEL_SE_PATH = "Assets/sprite/pause/label_se.dds";
	const char* const LABEL_OK_PATH = "Assets/sprite/pause/label_ok.dds";
	const char* const LABEL_BACK_PATH = "Assets/sprite/pause/label_back.dds";
	const char* const BUTTON_A_PATH = "Assets/sprite/pause/button_a.dds";
	const char* const BUTTON_B_PATH = "Assets/sprite/pause/button_b.dds";

	/** 背景の濃さ */
	constexpr float BACKGROUND_ALPHA = 0.65f;

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080） ----
	constexpr float PAUSE_TITLE_Y = 260.0f;
	constexpr float PAUSE_TITLE_HEIGHT = 170.0f;
	constexpr float MAIN_ITEM_HEIGHT = 95.0f;
	constexpr float MAIN_ITEM_Y[] = { 20.0f, -140.0f };
	constexpr float CURSOR_SIZE = 34.0f;
	constexpr float CURSOR_GAP = 50.0f;

	constexpr float SOUND_TITLE_Y = 300.0f;
	constexpr float SOUND_TITLE_HEIGHT = 120.0f;
	constexpr float SOUND_ROW_Y[] = { 110.0f, -30.0f, -170.0f };
	constexpr float SOUND_LABEL_HEIGHT = 70.0f;
	/** 文字の右端のX（右寄せにする） */
	constexpr float SOUND_LABEL_RIGHT_X = -230.0f;
	/** 目盛りの1つ目の中心のX と間隔・大きさ */
	constexpr float BLOCK_START_X = -150.0f;
	constexpr float BLOCK_PITCH = 62.0f;
	constexpr float BLOCK_SIZE = 46.0f;
	constexpr int BLOCK_COUNT = 10;

	constexpr float GUIDE_Y = -430.0f;
	constexpr float GUIDE_ICON_SIZE = 80.0f;
	constexpr float GUIDE_LABEL_HEIGHT = 50.0f;

	/** 選んでいる項目の大きさ */
	constexpr float SELECTED_SCALE = 1.12f;

	// ---- 色（COLOR_ で始まる名前は Windows のヘッダーのマクロとぶつかることがあるので PAUSE_ を付けている） ----
	const Vector4 PAUSE_COLOR_SELECTED(1.0f, 0.82f, 0.05f, 1.0f);
	const Vector4 PAUSE_COLOR_NORMAL(0.75f, 0.75f, 0.78f, 1.0f);
	const Vector4 PAUSE_COLOR_WHITE(1.0f, 1.0f, 1.0f, 1.0f);
	const Vector4 PAUSE_COLOR_BLOCK_OFF(0.25f, 0.25f, 0.28f, 0.9f);

	/** スティックを倒したとみなす値と、戻したとみなす値 */
	constexpr float STICK_ON = 0.6f;
	constexpr float STICK_OFF = 0.3f;
	/** 音量を1回で変える量 */
	constexpr float VOLUME_STEP = 0.1f;
}


namespace app
{
	namespace ui
	{
		PauseMenu::PauseMenu()
			: m_page(Page::Main)
			, m_mainIndex(0)
			, m_soundIndex(0)
			, m_isStickUp(false)
			, m_isStickDown(false)
			, m_isStickLeft(false)
			, m_isStickRight(false)
			, m_stickUpTrigger(false)
			, m_stickDownTrigger(false)
			, m_stickLeftTrigger(false)
			, m_stickRightTrigger(false)
		{}


		PauseMenu::~PauseMenu()
		{}


		void PauseMenu::Init()
		{
			// 画面いっぱいの背景（小さい黒い画像を引き伸ばす）
			m_background.render.Init(BACKGROUND_PATH, static_cast<float>(FRAME_BUFFER_W), static_cast<float>(FRAME_BUFFER_H));
			m_background.width = static_cast<float>(FRAME_BUFFER_W);
			m_background.height = static_cast<float>(FRAME_BUFFER_H);

			InitSprite(m_pauseTitle, PAUSE_TITLE_PATH, 973.0f, 235.0f, PAUSE_TITLE_HEIGHT);
			InitSprite(m_mainItems[static_cast<int>(MainItem::Sound)], ITEM_SOUND_PATH, 1155.0f, 268.0f, MAIN_ITEM_HEIGHT);
			InitSprite(m_mainItems[static_cast<int>(MainItem::Title)], ITEM_TITLE_PATH, 1025.0f, 265.0f, MAIN_ITEM_HEIGHT);
			InitSprite(m_cursor, BLOCK_PATH, 64.0f, 64.0f, CURSOR_SIZE);

			InitSprite(m_soundTitle, ITEM_SOUND_PATH, 1155.0f, 268.0f, SOUND_TITLE_HEIGHT);
			InitSprite(m_soundLabels[static_cast<int>(SoundItem::Master)], LABEL_MASTER_PATH, 442.0f, 96.0f, SOUND_LABEL_HEIGHT);
			InitSprite(m_soundLabels[static_cast<int>(SoundItem::BGM)], LABEL_BGM_PATH, 252.0f, 96.0f, SOUND_LABEL_HEIGHT);
			InitSprite(m_soundLabels[static_cast<int>(SoundItem::SE)], LABEL_SE_PATH, 147.0f, 96.0f, SOUND_LABEL_HEIGHT);
			for (auto& row : m_volumeBlocks)
			{
				for (Sprite2D& block : row)
				{
					InitSprite(block, BLOCK_PATH, 64.0f, 64.0f, BLOCK_SIZE);
				}
			}

			InitSprite(m_buttonA, BUTTON_A_PATH, 128.0f, 128.0f, GUIDE_ICON_SIZE);
			InitSprite(m_buttonB, BUTTON_B_PATH, 128.0f, 128.0f, GUIDE_ICON_SIZE);
			InitSprite(m_labelOk, LABEL_OK_PATH, 171.0f, 96.0f, GUIDE_LABEL_HEIGHT);
			InitSprite(m_labelBack, LABEL_BACK_PATH, 304.0f, 96.0f, GUIDE_LABEL_HEIGHT);

			// 動かない画像はここで置いておく
			Place(m_background, 0.0f, 0.0f, 1.0f, Vector4(1.0f, 1.0f, 1.0f, BACKGROUND_ALPHA));
			Place(m_pauseTitle, 0.0f, PAUSE_TITLE_Y, 1.0f, PAUSE_COLOR_WHITE);
			Place(m_soundTitle, 0.0f, SOUND_TITLE_Y, 1.0f, PAUSE_COLOR_SELECTED);
		}


		void PauseMenu::Open()
		{
			m_page = Page::Main;
			m_mainIndex = static_cast<int>(MainItem::Sound);
			m_soundIndex = static_cast<int>(SoundItem::Master);
			// 開いた瞬間にスティックを倒していても、勝手にカーソルが動かないように
			m_isStickUp = m_isStickDown = m_isStickLeft = m_isStickRight = true;
			m_stickUpTrigger = m_stickDownTrigger = m_stickLeftTrigger = m_stickRightTrigger = false;
			UpdateMainVisual();
		}


		PauseMenu::Result PauseMenu::Update()
		{
			UpdateStickTrigger();

			// - ボタンはどの画面からでもポーズ解除
			if (g_pad[0]->IsTrigger(enButtonSelect))
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonBack);
				return Result::Resume;
			}

			if (m_page == Page::Main)
			{
				return UpdateMain();
			}
			UpdateSound();
			return Result::None;
		}


		PauseMenu::Result PauseMenu::UpdateMain()
		{
			const int count = static_cast<int>(MainItem::Num);
			if (IsUpTrigger())
			{
				m_mainIndex = (m_mainIndex + count - 1) % count;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}
			else if (IsDownTrigger())
			{
				m_mainIndex = (m_mainIndex + 1) % count;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}
			UpdateMainVisual();

			if (g_pad[0]->IsTrigger(enButtonB))
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonBack);
				return Result::Resume;
			}
			if (g_pad[0]->IsTrigger(enButtonA))
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
				if (GetSelectedMainItem() == MainItem::Title)
				{
					return Result::GoToTitle;
				}
				// サウンドオプションへ
				m_page = Page::Sound;
				m_soundIndex = static_cast<int>(SoundItem::Master);
				UpdateSoundVisual();
			}
			return Result::None;
		}


		void PauseMenu::UpdateSound()
		{
			const int count = static_cast<int>(SoundItem::Num);
			if (IsUpTrigger())
			{
				m_soundIndex = (m_soundIndex + count - 1) % count;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}
			else if (IsDownTrigger())
			{
				m_soundIndex = (m_soundIndex + 1) % count;
				SoundManager::Get().PlaySE(enSoundKind_CursorMove);
			}

			// 左右で音量を変える（10段階。丸めて誤差がたまらないようにする）
			const SoundItem item = GetSelectedSoundItem();
			const float volume = GetVolume(item);
			if (IsLeftTrigger() || IsRightTrigger())
			{
				const float direction = IsRightTrigger() ? 1.0f : -1.0f;
				const float next = math::Clamp01(std::round(volume / VOLUME_STEP + direction) * VOLUME_STEP);
				if (next != volume)
				{
					SetVolume(item, next);
					// SE の音量はすぐ聞き比べられるように、変えたあとの音量で鳴らす
					SoundManager::Get().PlaySE(enSoundKind_CursorMove);
				}
			}

			if (g_pad[0]->IsTrigger(enButtonB))
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonBack);
				m_page = Page::Main;
				UpdateMainVisual();
				return;
			}
			UpdateSoundVisual();
		}


		void PauseMenu::UpdateMainVisual()
		{
			for (int i = 0; i < static_cast<int>(MainItem::Num); ++i)
			{
				const bool isSelected = i == m_mainIndex;
				Place(m_mainItems[i], 0.0f, MAIN_ITEM_Y[i], isSelected ? SELECTED_SCALE : 1.0f, isSelected ? PAUSE_COLOR_SELECTED : PAUSE_COLOR_NORMAL);
			}
			// 選んでいる項目の左にひし形の印
			const Sprite2D& selected = m_mainItems[m_mainIndex];
			const float cursorX = -selected.width * SELECTED_SCALE * 0.5f - CURSOR_GAP;
			Place(m_cursor, cursorX, MAIN_ITEM_Y[m_mainIndex], 1.0f, PAUSE_COLOR_SELECTED, 45.0f);

			// 案内：A OK / B BACK（右下）
			Place(m_buttonA, 430.0f, GUIDE_Y, 1.0f, PAUSE_COLOR_WHITE);
			Place(m_labelOk, 430.0f + GUIDE_ICON_SIZE * 0.5f + 10.0f + m_labelOk.width * 0.5f, GUIDE_Y, 1.0f, PAUSE_COLOR_WHITE);
			Place(m_buttonB, 640.0f, GUIDE_Y, 1.0f, PAUSE_COLOR_WHITE);
			Place(m_labelBack, 640.0f + GUIDE_ICON_SIZE * 0.5f + 10.0f + m_labelBack.width * 0.5f, GUIDE_Y, 1.0f, PAUSE_COLOR_WHITE);
		}


		void PauseMenu::UpdateSoundVisual()
		{
			for (int row = 0; row < static_cast<int>(SoundItem::Num); ++row)
			{
				const bool isSelected = row == m_soundIndex;
				Sprite2D& label = m_soundLabels[row];
				// 文字は右寄せ
				Place(label, SOUND_LABEL_RIGHT_X - label.width * 0.5f, SOUND_ROW_Y[row], 1.0f, isSelected ? PAUSE_COLOR_SELECTED : PAUSE_COLOR_NORMAL);

				// 目盛り：音量の分だけ光らせる
				const int litCount = static_cast<int>(std::round(GetVolume(static_cast<SoundItem>(row)) * BLOCK_COUNT));
				for (int i = 0; i < BLOCK_COUNT; ++i)
				{
					const bool isLit = i < litCount;
					const Vector4 color = isLit ? (isSelected ? PAUSE_COLOR_SELECTED : PAUSE_COLOR_WHITE) : PAUSE_COLOR_BLOCK_OFF;
					Place(m_volumeBlocks[row][i], BLOCK_START_X + BLOCK_PITCH * static_cast<float>(i), SOUND_ROW_Y[row], 1.0f, color);
				}
			}
			// 選んでいる行の左にひし形の印
			const Sprite2D& label = m_soundLabels[m_soundIndex];
			Place(m_cursor, SOUND_LABEL_RIGHT_X - label.width - CURSOR_GAP, SOUND_ROW_Y[m_soundIndex], 1.0f, PAUSE_COLOR_SELECTED, 45.0f);

			// 案内：B BACK（右下）
			Place(m_buttonB, 640.0f, GUIDE_Y, 1.0f, PAUSE_COLOR_WHITE);
			Place(m_labelBack, 640.0f + GUIDE_ICON_SIZE * 0.5f + 10.0f + m_labelBack.width * 0.5f, GUIDE_Y, 1.0f, PAUSE_COLOR_WHITE);
		}


		void PauseMenu::Render(RenderContext& rc)
		{
			// 描いた順に重なる（背景がいちばん下）
			m_background.render.Draw(rc);
			if (m_page == Page::Main)
			{
				m_pauseTitle.render.Draw(rc);
				for (Sprite2D& item : m_mainItems)
				{
					item.render.Draw(rc);
				}
				m_cursor.render.Draw(rc);
				m_buttonA.render.Draw(rc);
				m_labelOk.render.Draw(rc);
				m_buttonB.render.Draw(rc);
				m_labelBack.render.Draw(rc);
				return;
			}
			m_soundTitle.render.Draw(rc);
			for (int row = 0; row < static_cast<int>(SoundItem::Num); ++row)
			{
				m_soundLabels[row].render.Draw(rc);
				for (Sprite2D& block : m_volumeBlocks[row])
				{
					block.render.Draw(rc);
				}
			}
			m_cursor.render.Draw(rc);
			m_buttonB.render.Draw(rc);
			m_labelBack.render.Draw(rc);
		}


		void PauseMenu::InitSprite(Sprite2D& sprite, const char* path, const float pixelWidth, const float pixelHeight, const float displayHeight)
		{
			sprite.height = displayHeight;
			sprite.width = displayHeight * pixelWidth / pixelHeight;
			sprite.render.Init(path, sprite.width, sprite.height);
		}


		void PauseMenu::Place(Sprite2D& sprite, const float x, const float y, const float scale, const Vector4& color, const float rotationDeg)
		{
			Quaternion rotation;
			rotation.SetRotationDegZ(rotationDeg);
			sprite.render.SetPosition(Vector3(x, y, 0.0f));
			sprite.render.SetRotation(rotation);
			sprite.render.SetScale(Vector3(scale, scale, 1.0f));
			sprite.render.SetMulColor(color);
			sprite.render.Update();
		}


		bool PauseMenu::IsUpTrigger() { return g_pad[0]->IsTrigger(enButtonUp) || m_stickUpTrigger; }
		bool PauseMenu::IsDownTrigger() { return g_pad[0]->IsTrigger(enButtonDown) || m_stickDownTrigger; }
		bool PauseMenu::IsLeftTrigger() { return g_pad[0]->IsTrigger(enButtonLeft) || m_stickLeftTrigger; }
		bool PauseMenu::IsRightTrigger() { return g_pad[0]->IsTrigger(enButtonRight) || m_stickRightTrigger; }


		void PauseMenu::UpdateStickTrigger()
		{
			// 倒した瞬間だけ true（戻すまでは次の入力にならない）
			auto edge = [](const float value, bool& isHeld, bool& trigger)
			{
				trigger = false;
				if (!isHeld && value >= STICK_ON)
				{
					isHeld = true;
					trigger = true;
				}
				else if (isHeld && value <= STICK_OFF)
				{
					isHeld = false;
				}
			};
			const float x = g_pad[0]->GetLStickXF();
			const float y = g_pad[0]->GetLStickYF();
			edge(y, m_isStickUp, m_stickUpTrigger);
			edge(-y, m_isStickDown, m_stickDownTrigger);
			edge(-x, m_isStickLeft, m_stickLeftTrigger);
			edge(x, m_isStickRight, m_stickRightTrigger);
		}


		float PauseMenu::GetVolume(const SoundItem item)
		{
			const SoundManager& sound = SoundManager::Get();
			switch (item)
			{
			case SoundItem::Master:	return sound.GetMasterVolume();
			case SoundItem::BGM:	return sound.GetBGMVolume();
			case SoundItem::SE:		return sound.GetSEVolume();
			default:				return 0.0f;
			}
		}


		void PauseMenu::SetVolume(const SoundItem item, const float volume)
		{
			SoundManager& sound = SoundManager::Get();
			switch (item)
			{
			case SoundItem::Master:	sound.SetMasterVolume(volume); break;
			case SoundItem::BGM:	sound.SetBGMVolume(volume); break;
			case SoundItem::SE:		sound.SetSEVolume(volume); break;
			default: break;
			}
		}
	}
}
