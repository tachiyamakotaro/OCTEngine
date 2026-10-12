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
}


namespace app
{
	TitleScene::TitleScene()
		: m_titleSprite()
		, m_isNextScene(false)
	{}


	TitleScene::~TitleScene()
	{}


	bool TitleScene::Start()
	{
		// 画面いっぱいに出す（スプライトの原点は画面の真ん中）
		m_titleSprite.Init(TITLE_SPRITE_PATH, static_cast<float>(FRAME_BUFFER_W), static_cast<float>(FRAME_BUFFER_H));
		m_titleSprite.Update();
		SoundManager::Get().PlayBGM(enSoundKind_Title);
		return true;
	}


	void TitleScene::Update()
	{
		// A ボタンでインゲームへ
		if (!m_isNextScene && g_pad[0]->IsTrigger(enButtonA))
		{
			SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
			SoundManager::Get().FadeOutBGM(NEXT_SCENE_FADE_TIME);
			m_isNextScene = true;
		}
	}


	void TitleScene::Render(RenderContext& rc)
	{
		m_titleSprite.Draw(rc);
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
}
