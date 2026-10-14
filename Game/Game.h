/**
 * @file Game.h
 * @brief ゲーム本体（シーンの管理を動かすだけ）
 * @details 中身はシーンに分けている。
 *          ・TitleScene  … タイトル（START でインゲームへ、SOUND で音量の設定、QUIT でゲームを終わる）
 *          ・DebugScene  … インゲーム（ステージ・プレイヤー・エディタ・デバッグウィンドウ。ゴールでリザルトへ）
 *          ・ResultScene … クリア画面（A でタイトルへ）
 */
#pragma once
#include "Source/Core/DebugUI.h"


class Game : public IGameObject
{
public:
	Game();
	~Game() override;

	bool Start() override;
	void Update() override;
	void Render(RenderContext& rc) override;

	/**
	 * @brief ImGui（デバッグウィンドウ・エディタ・当たり判定の表示など）を画面に出すか
	 * @details 中身は app::debug::IsDebugUIVisible()（Source/Core/DebugUI.h）。
	 *          false の間はウィンドウを1つも作らないので、見えないウィンドウを触ってしまうこともない。
	 *          main.cpp では、これが false のときは ImGui_ImplDX12_RenderDrawData を呼ばない。
	 *          最初は false。Debug ビルドは F4 で切り替えられる。Release ビルドはずっと false。
	 */
	static bool IsImGuiVisible() { return app::debug::IsDebugUIVisible(); }
	static void SetImGuiVisible(const bool isVisible) { app::debug::SetDebugUIVisible(isVisible); }
};
