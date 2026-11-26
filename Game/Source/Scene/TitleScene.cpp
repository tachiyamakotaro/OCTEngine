/**
 * @file TitleScene.cpp
 * @brief タイトルシーン
 */
#include "stdafx.h"
#include "StageIntroScene.h"
#include "Source/PlayLog/PlayLog.h"
#include "Source/Save/SaveData.h"
#include "Source/Sound/SoundManager.h"
#include "Source/Stage/StageCatalog.h"
#include "TitleScene.h"


namespace
{
	/** インゲームへ移るときのフェードの時間（秒） */
	constexpr float NEXT_SCENE_FADE_TIME = 0.6f;
	/** ゲームを終わるときのフェードの時間（秒） */
	constexpr float QUIT_FADE_TIME = 0.8f;
}


namespace app
{
	TitleScene::TitleScene()
		: m_director()
		, m_menu()
		, m_isNextScene(false)
		, m_isQuit(false)
	{}


	TitleScene::~TitleScene()
	{}


	bool TitleScene::Start()
	{
		m_director.Init();
		m_menu.Init();
		// 続きの記録があれば START の中の CONTINUE を選べる（どこから続くかも「STAGE 〇」で出す）
		m_menu.Open(save::LoadProgress());
		SoundManager::Get().PlayBGM(enSoundKind_Title);
		return true;
	}


	void TitleScene::Update()
	{
		m_director.Update(g_gameTime->GetFrameDeltaTime());

		switch (m_menu.Update())
		{
		case ui::TitleMenu::Result::ContinueGame:
		{
			// 続きのステージから遊ぶ。プレイログは前のプレイの続きにする（同じファイルに書き直す）
			stage::SetCurrentStageIndex(m_menu.GetStartStage());
			playlog::PlayLog::Get().ContinuePlay();
			m_director.BeginEnter();
			break;
		}

		case ui::TitleMenu::Result::StartGame:
			// 選んだステージ（NEW GAME なら最初のステージ、ステージセレクトなら選んだステージ）から、新しいプレイとして遊ぶ。
			// 続きの記録とプレイログの続きは消す（ログは新しいファイルになる。続きの記録はインゲームが始まるときに書き直される）
			save::EraseProgress();
			playlog::PlayLog::Get().DiscardContinue();
			stage::SetCurrentStageIndex(m_menu.GetStartStage());
			m_director.BeginEnter();
			break;

		case ui::TitleMenu::Result::QuitGame:
			SoundManager::Get().FadeOutBGM(QUIT_FADE_TIME);
			m_isQuit = true;
			break;

		default:
			break;
		}

		// 扉が開いて中の光が見えたら、次のシーンへ
		if (!m_isNextScene && m_director.IsEnterFinished())
		{
			SoundManager::Get().FadeOutBGM(NEXT_SCENE_FADE_TIME);
			m_isNextScene = true;
		}
	}


	void TitleScene::PauseUpdate()
	{
		// フェード中も、演出と、決定した項目の点滅は続ける
		m_director.Update(g_gameTime->GetFrameDeltaTime());
		m_menu.Animate();
	}


	void TitleScene::Render(RenderContext& rc)
	{
		m_director.Draw(rc);
		m_menu.Render(rc);
	}


	bool TitleScene::RequestScene(uint32_t& id, float& fadeTime)
	{
		if (m_isNextScene)
		{
			// 「STAGE 〇」を出してからインゲームへ
			id = StageIntroScene::ID();
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
