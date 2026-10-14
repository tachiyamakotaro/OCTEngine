/**
 * @file TitleScene.cpp
 * @brief タイトルシーン
 */
#include "stdafx.h"
#include "TitleScene.h"
#include "DebugScene.h"
#include "Source/Sound/SoundManager.h"


namespace
{
	/** タイトルの画像（1920x1080 の DDS） */
	const char* const TITLE_SPRITE_PATH = "Assets/sprite/title.dds";
	/** インゲームへ移るときのフェードの時間（秒） */
	constexpr float NEXT_SCENE_FADE_TIME = 0.6f;
	/** ゲームを終わるときのフェードの時間（秒） */
	constexpr float QUIT_FADE_TIME = 0.8f;
}


namespace app
{
	TitleScene::TitleScene()
		: m_titleSprite()
		, m_menu()
		, m_isNextScene(false)
		, m_isQuit(false)
	{}


	TitleScene::~TitleScene()
	{}


	bool TitleScene::Start()
	{
		// 画面いっぱいに出す（スプライトの原点は画面の真ん中）
		m_titleSprite.Init(TITLE_SPRITE_PATH, static_cast<float>(FRAME_BUFFER_W), static_cast<float>(FRAME_BUFFER_H));
		m_titleSprite.Update();
		m_menu.Init();
		m_menu.Open();
		SoundManager::Get().PlayBGM(enSoundKind_Title);
		return true;
	}


	void TitleScene::Update()
	{
		switch (m_menu.Update())
		{
		case ui::TitleMenu::Result::StartGame:
			SoundManager::Get().FadeOutBGM(NEXT_SCENE_FADE_TIME);
			m_isNextScene = true;
			break;

		case ui::TitleMenu::Result::QuitGame:
			SoundManager::Get().FadeOutBGM(QUIT_FADE_TIME);
			m_isQuit = true;
			break;

		default:
			break;
		}
	}


	void TitleScene::PauseUpdate()
	{
		// フェード中も、決定した項目の点滅は続ける
		m_menu.Animate();
	}


	void TitleScene::Render(RenderContext& rc)
	{
		m_titleSprite.Draw(rc);
		m_menu.Render(rc);
	}


	bool TitleScene::RequestScene(uint32_t& id, float& fadeTime)
	{
		if (m_isNextScene)
		{
			id = DebugScene::ID();
			fadeTime = NEXT_SCENE_FADE_TIME;
			return true;
		}
		return false;
	}


	bool TitleScene::RequestQuit(float& fadeTime)
	{
		if (m_isQuit)
		{
			fadeTime = QUIT_FADE_TIME;
			return true;
		}
		return false;
	}
}
