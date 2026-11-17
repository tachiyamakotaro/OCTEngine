/**
 * @file StageIntroScene.h
 * @brief ステージが始まる前のシーン（暗い画面に「STAGE 3」（最後のステージは「STAGE FINAL」）を出して、少したったらインゲームへ）
 * @details タイトルの START のあとと、ステージをクリアして次のステージへ進むときに通る。
 *          どのステージを出すかは StageCatalog の今のステージ（GetCurrentStageNumber）。
 *          A ボタンで早く進める。
 */
#pragma once
#include "Source/Scene/IScene.h"
#include "Source/UI/StageIntroUI.h"


namespace app
{
	/**
	 * @brief ステージが始まる前のシーン
	 */
	class StageIntroScene : public IScene
	{
		appScene(StageIntroScene);

	public:
		StageIntroScene();
		~StageIntroScene() override;

		bool Start() override;
		void Update() override;
		void Render(RenderContext& rc) override;
		bool RequestScene(uint32_t& id, float& fadeTime) override;

	private:
		/** 「STAGE 3」の表示 */
		ui::StageIntroUI m_ui;
		/** シーンが始まってからの時間 */
		float m_timer;
		/** 次のシーン（インゲーム）へ移るか */
		bool m_isNextScene;
	};
}
