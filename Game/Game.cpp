#include "stdafx.h"

#include "Game.h"


bool Game::Start()
{
	// Load resources and set up your objects here (called once).
	SpriteInitData initData;
	initData.m_ddsFilePath[0] = "Assets/modelData/utc_all2.DDS";
	initData.m_fxFilePath = "Assets/shader/sprite.fx";
	initData.m_height = 256;
	initData.m_width = 256;
	initData.m_alphaBlendMode = AlphaBlendMode_Add;

	m_sprite.Init(initData);
	return true;
}

void Game::Update()
{
	// Per-frame logic goes here.
	m_sprite.Update(
		{ 0.0f,0.0f,0.0f },
		Quaternion::Identity,
		{ 1.0f,1.0f,1.0f }
	);

}

void Game::Render(RenderContext& rc)
{
	// Your drawing code goes here.
	// K2EngineLow already cleared the screen to gray before this is called.
	m_sprite.Draw(rc);
}
