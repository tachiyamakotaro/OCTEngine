/**
 * @file TitleScene.h
 * @brief タイトルシーン（遺跡の入口の演出とメニュー。START の中で NEW GAME・CONTINUE・STAGE SELECT から選んで遊ぶ、QUIT でゲームを終わる）
 */
#pragma once
#include "Source/Scene/IScene.h"
#include "Source/Title/TitleDirector.h"
#include "Source/UI/TitleMenu.h"


namespace app
{
	/**
	 * @brief タイトルシーン
	 * @details ・タイトルに必要なクラスを呼ぶだけにする
	 *              title::TitleDirector … 遺跡の入口の 3D の景色と、プレイヤーが門へ入っていく演出
	 *              ui::TitleMenu        … ロゴ・ボタン・サウンドオプション
	 *          ・START の中で遊ぶステージを決めると演出に門へ入ってもらい、扉が開ききったら次のシーンへ移る（QUIT はすぐに終わる）
	 *          ・演出はフェード中も止めない
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
		/** 遺跡の入口の演出 */
		title::TitleDirector m_director;
		/** START / SOUND / QUIT のメニュー（ロゴとボタンの後ろの帯も出す） */
		ui::TitleMenu m_menu;
		/** 次のシーンへ移るか */
		bool m_isNextScene;
		/** ゲームを終わるか */
		bool m_isQuit;
	};
}
