/**
 * @file Game.h
 * @brief ゲーム本体（シーンの管理を動かすだけ）
 * @details 中身はシーンに分けている。
 *          ・TitleScene  … タイトル（A でインゲームへ）
 *          ・DebugScene  … インゲーム（ステージ・プレイヤー・エディタ・デバッグウィンドウ。ゴールでリザルトへ）
 *          ・ResultScene … クリア画面（A でタイトルへ）
 */
#pragma once


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
	 * @details main.cpp で、これが false のときは ImGui_ImplDX12_RenderDrawData を呼ばない（＝ImGui を一切描かない）。
	 *          最初は出さない（Debug でも Release でも）。F4 で出す / 消す。
	 */
	static bool IsImGuiVisible() { return s_isImGuiVisible; }
	static void SetImGuiVisible(const bool isVisible) { s_isImGuiVisible = isVisible; }


private:
	/** ImGui を画面に出すか */
	static bool s_isImGuiVisible;
};
