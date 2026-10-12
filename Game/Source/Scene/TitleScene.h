/**
 * @file TitleScene.h
 * @brief タイトルシーン（タイトルの画像を出して、A ボタンでインゲームへ）
 */
#pragma once
#include "Source/Scene/IScene.h"


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
		void Render(RenderContext& rc) override;
		bool RequestScene(uint32_t& id, float& fadeTime) override;


	private:
		/** タイトルの画像 */
		SpriteRender m_titleSprite;
		/** 次のシーンへ移るか */
		bool m_isNextScene;
	};
}
