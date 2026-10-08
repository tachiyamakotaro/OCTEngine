#include "stdafx.h"

#include "../k2EngineLow/graphics/SceneLight.h"
#include "Game.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"


bool Game::Start()
{
	// Load resources and set up your objects here (called once).
	m_animationClips[0].Load("Assets/animData/idle.tka");
	m_animationClips[0].SetLoopFlag(true);
	m_animationClips[1].Load("Assets/animData/walk.tka");
	m_animationClips[1].SetLoopFlag(true);


	m_modelRender.Init("Assets/modelData/unityChan.tkm", m_animationClips, 2, true, false, &SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()), enModelUpAxisY);
	m_groundRender.Init("Assets/modelData/ground.tkm", nullptr, 0, true, true, &SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()));
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

	if (g_pad[0]->IsPress(enButtonA))
	{
		m_modelRender.PlayAnimation(1, 0.5f);
	}
	else
	{
		m_modelRender.PlayAnimation(0, 0.5f);
	}

	Quaternion modelRot = m_modelRender.GetRotation();
	modelRot.AddRotationDegY(g_pad[0]->GetRStickXF());
	m_modelRender.SetRotation(modelRot);

	IMGUIRender();

	SceneLight::GetInstance()->Update();
	m_modelRender.Update();
	m_groundRender.Update();
}

void Game::Render(RenderContext& rc)
{
	// Your drawing code goes here.
	// K2EngineLow already cleared the screen to gray before this is called.
	m_modelRender.Draw(rc);
	m_groundRender.Draw(rc);
}

void Game::IMGUIRender()
{
	ImGui::Begin("Light");
	ImGui::SliderFloat3("Direction", &SceneLight::GetInstance()->GetSceneLight().direction.x, -1.0f, 1.0f);
	ImGui::ColorEdit3("Color", &SceneLight::GetInstance()->GetSceneLight().ligColor.x);
	ImGui::ColorEdit3("Ambient", &SceneLight::GetInstance()->GetSceneLight().ambient.x);
	ImGui::SliderFloat("Spec Pow", &SceneLight::GetInstance()->GetSceneLight().specPow, 0.0f, 200.0f);
	ImGui::SliderFloat("Shadow Bias", &SceneLight::GetInstance()->GetSceneLight().shadowBias, 0.0f, 0.005f, "%.4f");
	ImGui::End();

	ImGui::Begin("PointLights");
	ImGui::SliderInt("Count", &SceneLight::GetInstance()->GetSceneLight().numPtLights, 0, SceneLight::GetMaxPointLight());
	for (int i = 0; i < SceneLight::GetInstance()->GetSceneLight().numPtLights; i++)
	{
		ImGui::PushID(i);
		if (ImGui::TreeNode("", "Light %d", i))
		{
			ImGui::DragFloat3("Position", &SceneLight::GetInstance()->GetSceneLight().ptLights[i].position.x, 5.01f);
			ImGui::ColorEdit3("Color", &SceneLight::GetInstance()->GetSceneLight().ptLights[i].color.x);
			ImGui::DragFloat("Range", &SceneLight::GetInstance()->GetSceneLight().ptLights[i].range, 5.0f, 0.0f, 2000.0f);
			ImGui::DragFloat3("Direction", &SceneLight::GetInstance()->GetSceneLight().ptLights[i].direction.x, 0.01f, -1.0f, 1.0f);
			ImGui::SliderAngle("Angle", &SceneLight::GetInstance()->GetSceneLight().ptLights[i].angle, 0.0f, 90.0f);
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	ImGui::End();

	ImGui::Begin("Blur");
	ImGui::SliderFloat("Blur Power", &RenderingEngine::GetInstance()->GetScreenBlurPower(), 0.0f, 20.0f);
	ImGui::End();

	ImGui::Begin("Bloom");
	ImGui::Checkbox("Enable", &RenderingEngine::GetInstance()->IsEnableBloom());
	ImGui::SliderFloat("Threshold", &RenderingEngine::GetInstance()->GetBloomThreshold(), 0.0f, 3.0f);
	ImGui::SliderFloat("Intensity", &RenderingEngine::GetInstance()->GetBloomIntensity(), 0.0f, 3.0f);
	ImGui::End();

	auto* renderingEngine = RenderingEngine::GetInstance();
	ImGui::Checkbox("DoF", &renderingEngine->IsEnableDof());
	ImGui::SliderFloat("FocusDistance", &renderingEngine->GetFocusDistance(), 0.0f, 3000.0f);   // ピント送り！
	ImGui::SliderFloat("FocusRange", &renderingEngine->GetFocusRange(), 10.0f, 2000.0f);
}
