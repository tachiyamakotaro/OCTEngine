/**
 * @file ResultScene.cpp
 * @brief リザルトシーン（宝物の間）
 */
#include "stdafx.h"
#include "ResultScene.h"
#include "TitleScene.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Result/TreasureRoom.h"
#include "Source/Sound/SoundManager.h"


namespace
{
	/** A ボタンを受け付け始めるまでの時間（クリアした勢いでボタンを連打していても、すぐに飛ばさないように） */
	constexpr float INPUT_WAIT_TIME = 1.0f;
	/** タイトルへ戻るときのフェードの時間（秒） */
	constexpr float NEXT_SCENE_FADE_TIME = 1.0f;
	/** カメラの近平面と遠平面（GameCamera と同じ） */
	constexpr float CAMERA_NEAR = 10.0f;
	constexpr float CAMERA_FAR = 20000.0f;
	/** 丸く開くときの中心（宝箱の少し上） */
	constexpr float IRIS_FOCUS_HEIGHT = 120.0f;
}


namespace app
{
	ResultScene::ResultScene()
		: m_layout()
		, m_timeline()
		, m_room()
		, m_stageRenderer()
		, m_backgroundRenderer()
		, m_treasure()
		, m_player()
		, m_ui()
		, m_irisWipe()
		, m_timer(0.0f)
		, m_isNextScene(false)
	{}


	ResultScene::~ResultScene()
	{
		// シーンのライトはどのシーンでも同じ物なので、宝石の光（ポイントライト）を残さない
		result::TreasureRenderer::ClearGemLight();
	}


	bool ResultScene::Start()
	{
		m_timeline.Init();

		// 部屋（地形ブロック）。地形と奥の背景はインゲームと同じ物で描く
		result::BuildTreasureRoom(m_room);
		stage::StageRenderer::Config rendererConfig;
		rendererConfig.modelPath = "Assets/modelData/Box.tkm";
		rendererConfig.upAxis = enModelUpAxisZ;
		rendererConfig.modelSize = Vector3(100.0f, 100.0f, 100.0f);
		rendererConfig.isPlane = false;
		rendererConfig.isPivotAtBottom = true;
		m_stageRenderer.Init(rendererConfig);
		m_backgroundRenderer.Init();

		m_treasure.Init(m_layout);
		m_player.Init(m_layout);
		m_ui.Init();
		m_irisWipe.Init();

		UpdateCamera();
		SoundManager::Get().PlayBGM(enSoundKind_Result);
		return true;
	}


	void ResultScene::Update()
	{
		const float deltaTime = g_gameTime->GetFrameDeltaTime();
		m_timer += deltaTime;

		// A ボタン：演出の途中なら最後まで飛ばす。案内が出ていればタイトルへ
		if (!m_isNextScene && m_timer >= INPUT_WAIT_TIME && g_pad[0]->IsTrigger(enButtonA))
		{
			if (m_timeline.CanLeave())
			{
				SoundManager::Get().PlaySE(enSoundKind_ButtonEnter);
				SoundManager::Get().FadeOutBGM(NEXT_SCENE_FADE_TIME);
				m_isNextScene = true;
			}
			else
			{
				m_timeline.SkipToEnd();
			}
		}

		PlaySounds(m_timeline.Update(deltaTime));

		UpdateCamera();
		m_stageRenderer.Update(m_room);
		m_backgroundRenderer.Update(m_room, *g_camera3D, true, deltaTime);
		m_treasure.Update(m_timeline);
		m_treasure.ApplyGemLight();
		m_player.Update(m_timeline);
		m_ui.Update(m_timeline);
		{
			const Vector3 focus(m_layout.chestX, m_layout.chestY + IRIS_FOCUS_HEIGHT, 0.0f);
			m_irisWipe.Update(m_timeline.GetIrisCloseRate(), focus);
		}
		SceneLight::GetInstance()->Update();
	}


	void ResultScene::Render(RenderContext& rc)
	{
		m_backgroundRenderer.Draw(rc);
		m_stageRenderer.Draw(rc);
		m_treasure.Draw(rc);
		m_player.Draw(rc);
		// 半透明の光は、ほかのモデルを全部登録したあとに登録する（エンジンは登録した順に描く）
		m_treasure.DrawGlow(rc);
		m_ui.Render(rc);
		// 丸く開くフェード（全部の画像より手前に出す）
		m_irisWipe.Draw(rc);
	}


	bool ResultScene::RequestScene(uint32_t& id, float& fadeTime)
	{
		if (m_isNextScene)
		{
			id = TitleScene::ID();
			fadeTime = NEXT_SCENE_FADE_TIME;
			return true;
		}
		return false;
	}


	void ResultScene::UpdateCamera()
	{
		// GameCamera と同じく、注視点の手前（-Z）から少し上から見る平行投影
		const float viewHeight = math::Lerp(m_layout.viewHeightStart, m_layout.viewHeightEnd, m_timeline.GetDollyRate());
		const float aspect = static_cast<float>(FRAME_BUFFER_W) / static_cast<float>(FRAME_BUFFER_H);
		g_camera3D->SetNear(CAMERA_NEAR);
		g_camera3D->SetFar(CAMERA_FAR);
		g_camera3D->SetUpdateProjMatrixFunc(Camera::enUpdateProjMatrixFunc_Ortho);
		g_camera3D->SetHeight(viewHeight);
		g_camera3D->SetWidth(viewHeight * aspect);

		const Vector3 target(m_layout.cameraTargetX, m_layout.cameraTargetY, 0.0f);
		g_camera3D->SetTarget(target);
		g_camera3D->SetPosition(Vector3(target.x, target.y + m_layout.cameraHeight, target.z - m_layout.cameraDistance));
	}


	void ResultScene::PlaySounds(const result::CelebrationTimeline::Events& events)
	{
		if (events.isLidOpened)
		{
			SoundManager::Get().PlaySE(enSoundKind_DoorOpen);
		}
		if (events.isGemShown)
		{
			SoundManager::Get().PlaySE(enSoundKind_Goal);
		}
		if (events.isHopped)
		{
			SoundManager::Get().PlaySE(enSoundKind_PlayerJump);
		}
	}
}
