/**
 * @file ResultScene.cpp
 * @brief リザルトシーン
 */
#include "stdafx.h"
#include "ResultScene.h"
#include "TitleScene.h"
#include "Source/Sound/SoundManager.h"


namespace
{
	/** クリアの画像（1920x1080 の DDS） */
	const char* const CLEAR_SPRITE_PATH = "Assets/sprite/gameclear.dds";
	/** A ボタンを受け付け始めるまでの時間（クリアした勢いでボタンを連打していても、すぐに飛ばさないように） */
	constexpr float INPUT_WAIT_TIME = 1.0f;
	/** タイトルへ戻るときのフェードの時間（秒） */
	constexpr float NEXT_SCENE_FADE_TIME = 1.0f;
}


namespace app
{
	ResultScene::ResultScene()
		: m_clearSprite()
		, m_timer(0.0f)
		, m_isNextScene(false)
	{}


	ResultScene::~ResultScene()
	{}


	bool ResultScene::Start()
	{
		m_clearSprite.Init(CLEAR_SPRITE_PATH, static_cast<float>(FRAME_BUFFER_W), static_cast<float>(FRAME_BUFFER_H));
		m_clearSprite.Update();
		SoundManager::Get().PlayBGM(enSoundKind_Result);
		return true;
	}


	void ResultScene::Update()
	{
		m_timer += g_gameTime->GetFrameDeltaTime();
		// A ボタンでタイトルへ
		if (!m_isNextScene && m_timer >= INPUT_WAIT_TIME && g_pad[0]->IsTrigger(enButtonA))
		{
			SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
			SoundManager::Get().FadeOutBGM(NEXT_SCENE_FADE_TIME);
			m_isNextScene = true;
		}
	}


	void ResultScene::Render(RenderContext& rc)
	{
		m_clearSprite.Draw(rc);
	}


	bool ResultScene::RequestScene(uint32_t& id, float& fadeTime)
	{
		if (m_isNextScene)
		{
			id = TitleScene::ID();
			fadeTime = NEXT_SCENE_FADE_TIME;
			return true;
		}
		return false;
	}
}
