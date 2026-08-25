#include "stdafx.h"

#include "Game.h"


bool Game::Start()
{
	// Load resources and set up your objects here (called once).
	m_light.ambient = { 0.6f, 0.6f, 0.6f };
	m_light.direction = { 1.0f, -1.0f, 0.0f };
	m_light.direction.Normalize();
	m_light.ligColor = { 0.6f, 0.7f, 0.4f };
	m_light.specPow = 0.5f;
	m_light.specIntensity = 5.0f;
	m_modelRender.Init("Assets/modelData/unityChan.tkm", &m_light, sizeof(m_light));
	m_modelRender.SetPosition({ 0.0f,0.0f,100.0f });

	return true;
}

void Game::Update()
{
	// Per-frame logic goes here.
	m_light.eyePos = g_camera3D->GetPosition();

	float modelXPos = m_modelRender.GetPosition().x;
	float modelYPos = m_modelRender.GetPosition().y;
	float modelZPos = m_modelRender.GetPosition().z;
	if (g_pad[0]->IsPress(enButtonB))
	{
		m_modelRender.SetPosition({ modelXPos += g_pad[0]->GetLStickXF(), modelYPos += g_pad[0]->GetLStickYF(), modelZPos });
	}
	else
	{
		m_modelRender.SetPosition({ modelXPos += g_pad[0]->GetLStickXF(), modelYPos , modelZPos += g_pad[0]->GetLStickYF() });
	}

	Quaternion modelRot = m_modelRender.GetRotation();
	modelRot.AddRotationDegY(g_pad[0]->GetRStickXF());
	m_modelRender.SetRotation(modelRot);

	m_modelRender.Update();

}

void Game::Render(RenderContext& rc)
{
	// Your drawing code goes here.
	// K2EngineLow already cleared the screen to gray before this is called.
	m_modelRender.Draw(rc);
}
