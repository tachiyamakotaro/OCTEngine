/**
 * @file SceneManager.cpp
 * @brief シーンの管理
 */
#include "stdafx.h"
#include "DebugScene.h"
#include "ResultScene.h"
#include "SceneManager.h"
#include "Source/Core/Fade.h"
#include "Source/Sound/SoundManager.h"
#include "TitleScene.h"


namespace app
{
	SceneManager* SceneManager::m_instance = nullptr;


	namespace
	{
		/** ポーズ画面からタイトルへ戻るときのフェードの時間 */
		constexpr float PAUSE_TO_TITLE_FADE_TIME = 1.0f;
		constexpr float PAUSE_TO_QUIT_FADE_TIME = 0.8f;
	}


	SceneManager::SceneManager()
	{
		// ポーズ画面の画像を読み込む
		m_pauseMenu.Init();

		// ここでシーンを登録する
		AddSceneMap<TitleScene>();
		AddSceneMap<DebugScene>();
		AddSceneMap<ResultScene>();

		// 最初のシーン（インゲームから始めたいときは DebugScene::ID() にする）
		CreateScene(TitleScene::ID());
	}


	SceneManager::~SceneManager()
	{}


	void SceneManager::Update()
	{
		switch (m_transitionState)
		{
		case TransitionState::Idle:
			if (!m_currentScene)
			{
				break;
			}
			// ---- ポーズ中：シーンは止めて、ポーズ画面だけ動かす ----
			if (m_isPause)
			{
				m_currentScene->PauseUpdate();
				const ui::PauseMenu::Result result = m_pauseMenu.Update();
				if (result == ui::PauseMenu::Result::Resume)
				{
					SetPause(false);
				}
				else if (result == ui::PauseMenu::Result::GoToTitle)
				{
					// ポーズ画面を出したまま暗くして、タイトルへ
					SoundManager::Get().FadeOutBGM(PAUSE_TO_TITLE_FADE_TIME);
					StartTransition(TitleScene::ID(), PAUSE_TO_TITLE_FADE_TIME);
				}
				else if (result == ui::PauseMenu::Result::QuitGame)
				{
					// ポーズ画面を出したまま暗くして、ゲームを終わる
					SoundManager::Get().FadeOutBGM(PAUSE_TO_QUIT_FADE_TIME);
					StartQuit(PAUSE_TO_QUIT_FADE_TIME);
				}
				break;
			}
			// ---- -（Select）ボタンでポーズ ----
			if (m_currentScene->CanPause() && g_pad[0]->IsTrigger(enButtonSelect))
			{
				SetPause(true);
				m_pauseMenu.Open();
				m_currentScene->PauseUpdate();
				break;
			}
			m_currentScene->Update();
			// 次のシーンへ移りたいか
			{
				uint32_t nextSceneId = INVALID_SCENE_ID;
				float fadeDuration = 0.0f;
				if (m_currentScene->RequestScene(nextSceneId, fadeDuration))
				{
					StartTransition(nextSceneId, fadeDuration);
				}
				// ゲームを終わりたいか
				else if (m_currentScene->RequestQuit(fadeDuration))
				{
					StartQuit(fadeDuration);
				}
			}
			break;

		case TransitionState::FadingOut:
			// 暗くしている間は操作を受け付けない（演出だけ動かす）
			if (m_currentScene)
			{
				m_currentScene->PauseUpdate();
			}
			if (core::Fade::Get().IsFadeOutComplete())
			{
				// 真っ暗になったら今のシーンを消す（次のシーンは次の段階で作る）。ポーズもここで解除する
				SetPause(false);
				m_currentScene.reset();
				m_transitionState = TransitionState::LoadingScene;
			}
			break;

		case TransitionState::LoadingScene:
			if (!m_currentScene && m_nextSceneId != INVALID_SCENE_ID)
			{
				CreateScene(m_nextSceneId);
				m_nextSceneId = INVALID_SCENE_ID;
			}
			if (m_currentScene)
			{
				m_currentScene->Update();
			}
			// 読み込みが終わったら明るくする
			if (!m_currentScene || m_currentScene->IsLoaded())
			{
				core::Fade::Get().FadeIn(m_fadeDuration);
				m_transitionState = TransitionState::FadingIn;
			}
			break;

		case TransitionState::FadingIn:
			if (m_currentScene)
			{
				m_currentScene->Update();
			}
			if (!core::Fade::Get().IsFading())
			{
				m_fadeDuration = 0.0f;
				m_transitionState = TransitionState::Idle;
			}
			break;

		case TransitionState::Quitting:
			// 暗くしている間は操作を受け付けない（演出だけ動かす）
			if (m_currentScene)
			{
				m_currentScene->PauseUpdate();
			}
			// 真っ暗になったら終わる（WM_QUIT を出すと main.cpp のゲームループが抜けて FinalizeGame が呼ばれる）
			if (!m_isQuitPosted && core::Fade::Get().IsFadeOutComplete())
			{
				PostQuitMessage(0);
				m_isQuitPosted = true;
			}
			break;
		}

		core::Fade::Get().Update();
	}


	void SceneManager::Render(RenderContext& rc)
	{
		if (m_currentScene)
		{
			m_currentScene->Render(rc);
		}
		// ポーズ画面はシーンの上、フェードはさらにその上（画像は描いた順に重なる）
		if (m_isPause)
		{
			m_pauseMenu.Render(rc);
		}
		core::Fade::Get().Render(rc);
	}


	void SceneManager::CreateScene(const uint32_t id)
	{
		auto it = m_sceneMap.find(id);
		K2_ASSERT(it != m_sceneMap.end(), "シーンが登録されていません。SceneManager のコンストラクタで AddSceneMap してください。");
		if (it == m_sceneMap.end())
		{
			return;
		}
		m_currentScene = it->second();
		m_currentScene->Start();
	}


	void SceneManager::SetPause(const bool isPause)
	{
		if (m_isPause == isPause)
		{
			return;
		}
		m_isPause = isPause;
		// 再生中のエフェクト（霧など）も止める
		EffectEngine::GetInstance()->SetPause(isPause);
		// 鳴り続けている SE（鏡を引きずる音など）も止める。BGM はそのまま
		if (SoundManager::IsCreated())
		{
			SoundManager::Get().SetPauseLoopSE(isPause);
		}
	}


	void SceneManager::StartTransition(const uint32_t nextSceneId, const float fadeDuration)
	{
		m_nextSceneId = nextSceneId;
		m_fadeDuration = fadeDuration;
		core::Fade::Get().FadeOut(fadeDuration);
		m_transitionState = TransitionState::FadingOut;
	}


	void SceneManager::StartQuit(const float fadeDuration)
	{
		m_nextSceneId = INVALID_SCENE_ID;
		m_fadeDuration = fadeDuration;
		core::Fade::Get().FadeOut(fadeDuration);
		m_transitionState = TransitionState::Quitting;
	}
}
