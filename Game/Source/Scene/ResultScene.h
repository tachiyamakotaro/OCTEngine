/**
 * @file ResultScene.h
 * @brief リザルトシーン（クリアの画像を出して、A ボタンでタイトルへ）
 */
#pragma once
#include "Source/Scene/IScene.h"


namespace app
{
	/**
	 * @brief リザルトシーン
	 */
	class ResultScene : public IScene
	{
		appScene(ResultScene);

	public:
		ResultScene();
		~ResultScene() override;

		bool Start() override;
		void Update() override;
		void Render(RenderContext& rc) override;
		bool RequestScene(uint32_t& id, float& fadeTime) override;


	private:
		/** クリアの画像 */
		SpriteRender m_clearSprite;
		/** シーンが始まってからの時間（すぐに A を受け付けないため） */
		float m_timer;
		/** 次のシーンへ移るか */
		bool m_isNextScene;
	};
}
