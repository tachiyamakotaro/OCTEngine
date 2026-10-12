/**
 * @file Game.cpp
 * @brief ゲーム本体（シーンの管理を動かすだけ）
 */
#include "stdafx.h"
#include "Game.h"
#include "Source/Core/Fade.h"
#include "Source/Scene/SceneManager.h"
#include "Source/Sound/SoundManager.h"
#include "imgui.h"


// ImGui は最初は出さない（Debug でも Release でも）。F4 で出す / 消す
bool Game::s_isImGuiVisible = false;


Game::Game()
{}


Game::~Game()
{
	// シーン（とその画像）を先に消してから、フェードを消す
	app::SceneManager::DestroyInstance();
	app::core::Fade::Delete();
	app::SoundManager::DestroyInstance();
}


bool Game::Start()
{
	// サウンド → フェード → シーンの管理 の順に作る（最初のシーン＝タイトルもここで作られる）
	app::SoundManager::CreateInstance();
	app::core::Fade::Create();
	app::SceneManager::CreateInstance();
	return true;
}


void Game::Update()
{
	// F4：撮影モード（ImGui を全部消す / 戻す）。文字入力中は無視する
	if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F4, false))
	{
		s_isImGuiVisible = !s_isImGuiVisible;
	}

	app::SceneManager::GetInstance()->Update();
	// シーンがリクエストした SE をまとめて鳴らす
	app::SoundManager::Get().Update();
}


void Game::Render(RenderContext& rc)
{
	app::SceneManager::GetInstance()->Render(rc);
}
