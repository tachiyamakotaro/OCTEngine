#include "stdafx.h"

#include "Game.h"


bool Game::Start()
{
	// Load resources and set up your objects here (called once).
	m_spriteRender.Init("Assets/modelData/utc_all2.DDS", 256.0f, 256.0f);

	return true;
}

void Game::Update()
{
	// Per-frame logic goes here.
	m_spriteRender.SetPosition({ 10.0f,0.0f,0.0f });
	m_spriteRender.Update();

}

void Game::Render(RenderContext& rc)
{
	// Your drawing code goes here.
	// K2EngineLow already cleared the screen to gray before this is called.
	m_spriteRender.Draw(rc);
}
