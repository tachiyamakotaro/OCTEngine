/**
 * @file SceneManager.h
 * @brief シーンの管理（タイトル → インゲーム → リザルト → タイトル …）
 * @details シーンが RequestScene で次のシーンを求めると、
 *          画面を暗くする（フェードアウト）→ 今のシーンを消して次のシーンを作る → 明るくする（フェードイン）
 *          の順に切り替える。フェードは core::Fade（真っ黒な画像を画面全体に出す）を使う。
 *          core::Fade と SoundManager は SceneManager より先に作っておくこと（Game::Start で作っている）。
 *
 *          ポーズ：CanPause() が true のシーン（インゲーム）で -（Select）ボタンを押すとポーズする。
 *          ポーズ中はシーンの Update を呼ばず（PauseUpdate だけ）、エフェクトも止める。ポーズ画面は ui::PauseMenu。
 *
 *          ゲームを終わる：シーンが RequestQuit で true を返すと、画面を暗くしてからゲームを終わる（StartQuit）。
 *          BGM のフェードアウトは、シーンを切り替えるときと同じく呼んだ側でする。
 */
#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include "Source/Scene/IScene.h"
#include "Source/UI/PauseMenu.h"


namespace app
{
	/**
	 * @brief シーンの管理
	 */
	class SceneManager
	{
	public:
		/** 毎フレームの更新 */
		void Update();
		/** 描画 */
		void Render(RenderContext& rc);


	public:
		/** @brief シングルトンインスタンスを生成（最初のシーン＝タイトルも作る） */
		static void CreateInstance()
		{
			if (m_instance == nullptr)
			{
				m_instance = new SceneManager();
			}
		}
		/** @brief シングルトンインスタンスを取得 */
		static SceneManager* GetInstance() { return m_instance; }
		/** @brief シングルトンインスタンスを破棄 */
		static void DestroyInstance()
		{
			delete m_instance;
			m_instance = nullptr;
		}


	public:
		/** 今のシーン */
		inline IScene* GetCurrentScene() const { return m_currentScene.get(); }
		/** ポーズ中か */
		inline bool IsPause() const { return m_isPause; }
		/** ポーズ画面 */
		inline const ui::PauseMenu& GetPauseMenu() const { return m_pauseMenu; }
		/** シーンを切り替えている途中か */
		inline bool IsTransitioning() const { return m_transitionState != TransitionState::Idle; }


	private:
		/** シーン切り替えの状態 */
		enum class TransitionState
		{
			Idle,			//!< ふつうに遊んでいる
			FadingOut,		//!< 暗くしている
			LoadingScene,	//!< 真っ暗な間に次のシーンを作って、読み込みが終わるのを待つ
			FadingIn,		//!< 明るくしている（次のシーンはもう動いている）
			Quitting,		//!< ゲームを終わるために暗くしている（暗くなりきったら終わる）
		};


	private:
		SceneManager();
		~SceneManager();
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;

		/**
		 * @brief シーンを登録する（コンストラクタで呼ぶ）
		 */
		template<typename T>
		void AddSceneMap()
		{
			m_sceneMap.emplace(T::ID(), []() { return std::unique_ptr<IScene>(new T()); });
		}
		/** 番号のシーンを作って Start を呼ぶ */
		void CreateScene(uint32_t id);
		/** ポーズする・解除する（エフェクトも止める・動かす） */
		void SetPause(bool isPause);
		/** 次のシーンへの切り替えを始める */
		void StartTransition(uint32_t nextSceneId, float fadeDuration);
		/** 画面を暗くしてからゲームを終わる（ポーズ画面に「ゲームを終わる」を足すときもこれを呼ぶ） */
		void StartQuit(float fadeDuration);


	private:
		/** シーンの作り方（番号 → 作る関数） */
		std::map<uint32_t, std::function<std::unique_ptr<IScene>()>> m_sceneMap;
		/** 今のシーン */
		std::unique_ptr<IScene> m_currentScene;
		/** 次のシーンの番号 */
		uint32_t m_nextSceneId = INVALID_SCENE_ID;
		/** 切り替えの状態 */
		TransitionState m_transitionState = TransitionState::Idle;
		/** フェードの時間 */
		float m_fadeDuration = 0.0f;
		/** ポーズ中か */
		bool m_isPause = false;
		/** ゲームを終わる知らせ（WM_QUIT）をもう出したか */
		bool m_isQuitPosted = false;
		/** ポーズ画面 */
		ui::PauseMenu m_pauseMenu;

		/** シングルトンインスタンス */
		static SceneManager* m_instance;
	};
}
