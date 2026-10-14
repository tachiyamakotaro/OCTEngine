/**
 * @file TitleScene.h
 * @brief タイトルシーン（タイトルの画像とメニュー。START でインゲームへ、QUIT でゲームを終わる）
 */
#pragma once
#include "Source/Scene/IScene.h"
#include "Source/UI/TitleMenu.h"


namespace app
{
	/**
	 * @brief タイトルシーン
	 */
	class TitleScene : public IScene
	{
		appScene(TitleScene);

	public:
		TitleScene();
		~TitleScene() override;

		bool Start() override;
		void Update() override;
		void PauseUpdate() override;
		void Render(RenderContext& rc) override;
		bool RequestScene(uint32_t& id, float& fadeTime) override;
		bool RequestQuit(float& fadeTime) override;


	private:
		/** タイトルの画像 */
		SpriteRender m_titleSprite;
		/** START / SOUND / QUIT のメニュー */
		ui::TitleMenu m_menu;
		/** 次のシーンへ移るか */
		bool m_isNextScene;
		/** ゲームを終わるか */
		bool m_isQuit;
	};
}
