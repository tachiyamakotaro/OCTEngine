#include "stdafx.h"
#include "Game.h"

#include <cmath>
#include "imgui.h"
#include "Source/Actor/Character/CharacterStatus.h"
#include "Source/Actor/Character/CharacterStateMachine.h"
#include "Source/Stage/Stages/Stage01.h"


namespace
{
	/** プレイヤーの開始位置（足元） */
	const app::math::Vec2 PLAYER_START_POSITION = { 230.0f, 0.0f };
	/** これより下に落ちたら死亡 */
	constexpr float KILL_Y = -1000.0f;
	/** 死んでからリスタートするまでの時間 */
	constexpr float RESTART_DELAY = 2.0f;

	/** 注視点の高さ（足元から体の中ほど） */
	constexpr float CAMERA_TARGET_OFFSET_Y = 60.0f;
	/** カメラの距離の範囲 */
	constexpr float CAMERA_MIN_DISTANCE = 500.0f;
	constexpr float CAMERA_MAX_DISTANCE = 6000.0f;
}


Game::Game()
	: m_inputDevice(0)
	, m_playerStartPosition(PLAYER_START_POSITION)
	, m_respawnPosition(PLAYER_START_POSITION)
	, m_respawnName()
	, m_stageBodies()
	, m_cameraTarget(0.0f, 0.0f, 0.0f)
	, m_cameraDistance(1100.0f)
	, m_followDistance(1100.0f)
	, m_cameraHeight(150.0f)
	, m_cameraSmoothing(6.0f)
	, m_isAutoRestart(true)
	, m_deadTimer(0.0f)
	, m_isShowDebugWindow(true)
{}


bool Game::Start()
{
	// ステージ
	app::stage::BuildStage01(m_stage);

	// 地形の描画（今は黄色い床をブロックの上面に置く）
	app::stage::StageRenderer::Config rendererConfig;
	rendererConfig.modelPath = "Assets/modelData/Box.tkm";
	rendererConfig.upAxis = enModelUpAxisZ;
	rendererConfig.modelSize = Vector3(100.0f, 100.0f, 100.0f);
	rendererConfig.isPlane = false;
	rendererConfig.isPivotAtBottom = true;
	m_stageRenderer.Init(rendererConfig);

	// ギミックの描画（Assets/modelData/gimmick/ の仮モデル）
	m_gimmickRenderer.Init(app::stage::GimmickRenderer::Config());
	// モデルで描くようになったので、デバッグ表示は当たり判定の枠とキャラクターだけにする（Display で切り替えられる）
	m_overlay.isFillBlocks = false;
	m_overlay.isDrawGimmicks = false;
	m_overlay.isDrawLight = false;

	// プレイヤー（奥の鏡に映った姿も描けるようにしておく）
	m_player.SetInputDevice(&m_inputDevice);
	m_player.SetCollisionWorld(&m_stage);
	m_player.SetPosition(m_playerStartPosition);
	m_player.EnableReflection();

	// NewGO していないので、Start は自分で呼ぶ（モデルの読み込みとステートマシンの準備）
	m_player.Start();

	// エディタとデバッグ表示にキャラクターを登録
	m_stageEditor.AddCharacter("Player", &m_player);
	m_overlay.AddCharacter("Player", &m_player);

	// カメラ（遠くまで映るように）
	g_camera3D->SetNear(10.0f);
	g_camera3D->SetFar(20000.0f);
	UpdateCamera(0.0f, true);

	return true;
}


void Game::Update()
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();

	UpdateHotkeys();
	ApplyTuning();

	// 順番が大事：ステージ（スイッチ・光・扉・足場・霧）→ プレイヤー → 反射鏡の操作
	// スイッチは前のフレームのプレイヤーの位置で踏んだか調べる
	m_stageBodies.clear();
	m_stageBodies.push_back(m_player.GetController().GetBounds());
	m_stage.Update(deltaTime, m_stageBodies);
	{
		// 奥の鏡に映る姿（エディタで鏡を動かしても追従するよう毎フレーム渡す）
		const app::stage::BackMirror& backMirror = m_stage.GetBackMirror();
		m_player.SetReflectionPlane(backMirror.isEnabled, backMirror.planeZ);
	}
	m_player.Update();
	m_mirrorInteractor.Update(m_stage, m_player);

	UpdateCheckpoint();
	UpdateStageEvents();
	UpdateRestart(deltaTime);

	m_stageRenderer.Update(m_stage);
	m_gimmickRenderer.Update(m_stage);
	UpdateCamera(deltaTime, false);
	SceneLight::GetInstance()->Update();

	DrawImGui();
}


void Game::Render(RenderContext& rc)
{
	m_stageRenderer.Draw(rc);
	m_gimmickRenderer.Draw(rc);
	m_player.Render(rc);
}


void Game::ReloadStage()
{
	app::stage::BuildStage01(m_stage);
	// 作り直したらチェックポイントも最初から
	m_respawnPosition = m_playerStartPosition;
	m_respawnName.clear();
	RestartPlayers();
}


void Game::RestartPlayers()
{
	m_stage.ResetRuntimeState();
	m_mirrorInteractor.Reset();

	m_player.SetPosition(m_respawnPosition);
	m_player.SetFacing(1);

	// コントローラー・ステータス・ステートマシンを作り直す（モデルはそのまま）
	m_player.SetupCharacter();
	// 死亡ステートで止めた入力を戻す
	m_player.GetPlayerController().SetEnabled(true);

	m_deadTimer = 0.0f;
	UpdateCamera(0.0f, true);
}


void Game::UpdateCheckpoint()
{
	// 死んでいる間は更新しない（落ちていく途中でチェックポイントを通っても再開位置にしない）
	if (m_player.GetStatus()->IsDead())
	{
		return;
	}
	if (const app::stage::Checkpoint* checkpoint = m_stage.FindCheckpoint(m_player.GetController().GetBounds()))
	{
		m_respawnPosition = checkpoint->GetSpawnPosition();
		m_respawnName = checkpoint->name;
	}
}


void Game::UpdateStageEvents()
{
	// 霧が吹き出したフレーム：吹き出し口ごとに霧のエフェクトを出す
	std::vector<app::math::Vec2> ventPositions;
	for (const app::stage::MistEmitter& mist : m_stage.GetMistEmitters())
	{
		if (!mist.isFiredThisFrame)
		{
			continue;
		}
		m_stage.GetMistVentPositions(mist, ventPositions);
		for (const app::math::Vec2& vent : ventPositions)
		{
			// TODO: 霧のエフェクトを (vent.x, vent.y, 0) から下向きに再生する
			//       霧が下りる速さは mist.fallSpeed、吹いている時間は mist.duration、晴れるまでは mist.fadeTime
			(void)vent;
		}
	}

	// 鏡の足場が見えたフレーム：露がつくエフェクトを出す（そのあと露は足場に残す）
	for (const app::stage::MirrorPlatform& platform : m_stage.GetMirrorPlatforms())
	{
		if (!platform.isRevealedThisFrame)
		{
			continue;
		}
		// TODO: 露のエフェクトを足場の上面 (platform.rect) に再生する
	}
}


void Game::UpdateRestart(const float deltaTime)
{
	// 奈落に落ちたら死亡
	if (m_player.GetPosition().y < KILL_Y)
	{
		m_player.GetStatus()->Kill();
	}

	if (!m_player.GetStatus()->IsDead())
	{
		m_deadTimer = 0.0f;
		return;
	}

	m_deadTimer += deltaTime;
	if (m_isAutoRestart && m_deadTimer >= RESTART_DELAY)
	{
		RestartPlayers();
	}
}


void Game::UpdateCamera(const float deltaTime, const bool isImmediate)
{
	const app::math::Vec2& playerPos = m_player.GetPosition();

	// プレイヤーを追う（奥の鏡に映った姿も一緒に画面に入る）
	const Vector3 target(playerPos.x, playerPos.y + CAMERA_TARGET_OFFSET_Y, 0.0f);
	const float distance = app::math::Clamp(m_followDistance, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE);

	// なめらかに追いかける（フレームレートが変わっても同じ速さになる書き方）
	const float t = isImmediate ? 1.0f : 1.0f - std::exp(-m_cameraSmoothing * deltaTime);
	m_cameraTarget.x += (target.x - m_cameraTarget.x) * t;
	m_cameraTarget.y += (target.y - m_cameraTarget.y) * t;
	m_cameraTarget.z = 0.0f;
	m_cameraDistance += (distance - m_cameraDistance) * t;

	// 手前(-Z)から、少し上から見る
	g_camera3D->SetTarget(m_cameraTarget);
	g_camera3D->SetPosition(Vector3(m_cameraTarget.x, m_cameraTarget.y + m_cameraHeight, -m_cameraDistance));
}


void Game::ApplyTuning()
{
	// 重力などコントローラーに渡してある値は毎フレーム反映し直す（体の大きさはリスタートで反映）
	const app::actor::PlayerParameter& param = m_player.GetPlayerParameter();
	m_player.GetController().SetGravity(param.gravity);
	m_player.GetController().SetMaxFallSpeed(param.maxFallSpeed);
}


void Game::UpdateHotkeys()
{
	// 文字入力中はキー操作を無視する
	if (ImGui::GetIO().WantTextInput)
	{
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_F1, false))
	{
		m_stageEditor.isOpen = !m_stageEditor.isOpen;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
	{
		m_isShowDebugWindow = !m_isShowDebugWindow;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_F5, false))
	{
		RestartPlayers();
	}
}


void Game::DrawImGui()
{
	// ステージを3D画面に重ねて描く（ワールド座標 → 画面のピクセル座標）
	const auto projector = [](const app::math::Vec2& world, const float z, ImVec2& outScreen)
	{
		Vector4 clip(world.x, world.y, z, 1.0f);
		g_camera3D->GetViewProjectionMatrix().Apply(clip);
		if (clip.w <= 0.0001f)
		{
			// カメラの後ろ
			return false;
		}
		const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
		outScreen.x = (clip.x / clip.w * 0.5f + 0.5f) * displaySize.x;
		outScreen.y = (0.5f - clip.y / clip.w * 0.5f) * displaySize.y;
		return true;
	};
	m_overlay.Draw(ImGui::GetBackgroundDrawList(), m_stage, &m_mirrorInteractor, projector);

	m_stageEditor.Draw(m_stage);
	DrawDebugWindow();
}


void Game::DrawDebugWindow()
{
	if (!m_isShowDebugWindow)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(340.0f, 0.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Game (F2)", &m_isShowDebugWindow))
	{
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("F1: Stage Editor   F2: This window   F5: Restart");

	// ---- プレイヤーの状態 ----
	const app::actor::CharacterController& controller = m_player.GetController();
	ImGui::SeparatorText("Player");
	ImGui::Text("State : %s", m_player.GetStateMachine()->GetCurrentStateName());
	ImGui::Text("HP    : %d / %d", m_player.GetStatus()->GetHp(), m_player.GetStatus()->GetMaxHp());
	ImGui::Text("Pos   : (%.1f, %.1f)", m_player.GetPosition().x, m_player.GetPosition().y);
	ImGui::Text("Vel   : (%.1f, %.1f)", controller.GetVelocity().x, controller.GetVelocity().y);
	ImGui::Text("Ground: %s  Wall L/R: %s/%s", controller.IsOnGround() ? "yes" : "no",
		controller.IsTouchingWall(-1) ? "yes" : "no", controller.IsTouchingWall(1) ? "yes" : "no");

	if (ImGui::Button("Restart (F5)"))
	{
		RestartPlayers();
	}
	ImGui::SetItemTooltip("Keep the stage layout. Reset players, doors, lights and mist. Respawn at the last checkpoint.");
	ImGui::SameLine();
	if (ImGui::Button("Reload Stage01"))
	{
		ReloadStage();
	}
	ImGui::SetItemTooltip("Rebuild the stage from Stage01.cpp. Unsaved editor changes are lost.");
	ImGui::Checkbox("Auto restart on death", &m_isAutoRestart);
	ImGui::Text("Respawn: %s (%.0f, %.0f)", m_respawnName.empty() ? "Start" : m_respawnName.c_str(), m_respawnPosition.x, m_respawnPosition.y);

	// チェックポイントへワープ（部屋2などを最初から確かめたいとき）
	if (!m_stage.GetCheckpoints().empty())
	{
		ImGui::TextUnformatted("Warp:");
		if (ImGui::SmallButton("Start"))
		{
			m_respawnPosition = m_playerStartPosition;
			m_respawnName.clear();
			RestartPlayers();
		}
		for (const app::stage::Checkpoint& checkpoint : m_stage.GetCheckpoints())
		{
			ImGui::SameLine();
			ImGui::PushID(&checkpoint);
			if (ImGui::SmallButton(checkpoint.name.empty() ? "(no name)" : checkpoint.name.c_str()))
			{
				m_respawnPosition = checkpoint.GetSpawnPosition();
				m_respawnName = checkpoint.name;
				RestartPlayers();
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
	}

	// ---- 動きの調整 ----
	if (ImGui::CollapsingHeader("Tuning"))
	{
		app::actor::PlayerParameter& param = m_player.GetPlayerParameterForEdit();
		ImGui::DragFloat("Run Speed", &param.runSpeed, 5.0f, 0.0f, 3000.0f, "%.0f");
		ImGui::DragFloat("Jump Velocity", &param.jumpVelocity, 10.0f, 0.0f, 5000.0f, "%.0f");
		ImGui::DragFloat("Gravity", &param.gravity, 10.0f, -20000.0f, 0.0f, "%.0f");
		ImGui::DragFloat("Max Fall Speed", &param.maxFallSpeed, 10.0f, 0.0f, 10000.0f, "%.0f");
		ImGui::DragFloat("Ledge Reach", &param.ledgeGrabReach, 1.0f, 0.0f, 200.0f, "%.0f");
		ImGui::DragFloat("Ledge Above", &param.ledgeGrabAbove, 1.0f, 0.0f, 200.0f, "%.0f");
		ImGui::DragFloat("Climb Duration", &param.climbUpDuration, 0.01f, 0.05f, 3.0f, "%.2f s");
		ImGui::DragFloat("Hard Landing Height", &param.hardLandingHeight, 5.0f, 0.0f, 3000.0f, "%.0f");
		ImGui::DragFloat("Deadly Fall Height", &param.deadlyFallHeight, 5.0f, 0.0f, 5000.0f, "%.0f");
		ImGui::Checkbox("Auto Ledge Grab", &param.isAutoLedgeGrab);
		ImGui::Separator();
		ImGui::DragFloat("Model Yaw Offset", &param.modelYawOffsetDeg, 1.0f, -180.0f, 180.0f, "%.0f deg");
		ImGui::SetItemTooltip("Set 180 if Unity-chan faces backwards.");
		ImGui::DragFloat("Body Width", &param.bodyWidth, 1.0f, 10.0f, 300.0f, "%.0f");
		ImGui::DragFloat("Body Height", &param.bodyHeight, 1.0f, 10.0f, 400.0f, "%.0f");
		ImGui::DragFloat("Crouch Height", &param.crouchHeight, 1.0f, 10.0f, 400.0f, "%.0f");
		ImGui::TextDisabled("Body size is applied on Restart.");
	}

	// ---- カメラ ----
	if (ImGui::CollapsingHeader("Camera"))
	{
		ImGui::DragFloat("Distance", &m_followDistance, 10.0f, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE, "%.0f");
		ImGui::DragFloat("Height", &m_cameraHeight, 5.0f, -2000.0f, 2000.0f, "%.0f");
		ImGui::DragFloat("Smoothing", &m_cameraSmoothing, 0.1f, 0.1f, 30.0f, "%.1f");
	}

	// ---- 表示 ----
	if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
	{
		m_overlay.DrawSettingsUI();
		ImGui::DragFloat("Floor Depth", &m_stageRenderer.GetConfig().depth, 5.0f, 10.0f, 5000.0f, "%.0f");
		ImGui::Checkbox("Gimmick Reflection", &m_gimmickRenderer.GetConfig().isDrawReflection);
		ImGui::SetItemTooltip("Keep the Back Mirror's Plane Z at half of this so the mirror sits at the back edge of the floor.");
		app::stage::BackMirror& backMirror = m_stage.GetBackMirror();
		ImGui::Checkbox("Back Mirror", &backMirror.isEnabled);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat("Plane Z", &backMirror.planeZ, 1.0f, -5000.0f, 5000.0f, "%.0f");
	}

	ImGui::End();
}
