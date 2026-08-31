#include "stdafx.h"

#include "../k2EngineLow/graphics/SceneLight.h"
#include "Game.h"


bool Game::Start()
{
	// Load resources and set up your objects here (called once).


	m_modelRender.Init("Assets/modelData/unityChan.tkm", true, false, &SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()));
	m_groundRender.Init("Assets/modelData/ground.tkm", false, true, &SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()));
	m_modelRender.SetPosition({ 0.0f,0.0f,10.0f });
	m_groundRender.SetPosition({ 0.0f,-10.0f,0.0f });

	return true;
}

void Game::Update()
{
	// Per-frame logic goes here.

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
	m_groundRender.Draw(rc);
}
