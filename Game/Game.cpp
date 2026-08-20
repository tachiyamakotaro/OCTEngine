#include "stdafx.h"

#include "Game.h"


bool Game::Start()
{
	// Load resources and set up your objects here (called once).
	m_light.ambient = { 0.3f, 0.3f, 0.3f };
	m_light.direction = { 0.0f, 0.0f, 1.0f };
	m_light.direction.Normalize();
	m_light.ligColor = { 10.0f, 0.0f, 0.0f };
	m_modelRender.Init("Assets/modelData/unityChan.tkm", &m_light, sizeof(m_light));
	m_modelRender.SetPosition({ 0.0f,0.0f,100.0f });

	return true;
}

void Game::Update()
{
	// Per-frame logic goes here.
	m_modelRender.Update();

}

void Game::Render(RenderContext& rc)
{
	// Your drawing code goes here.
	// K2EngineLow already cleared the screen to gray before this is called.
	m_modelRender.Draw(rc);
}
