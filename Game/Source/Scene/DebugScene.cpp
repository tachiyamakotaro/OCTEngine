/**
 * @file DebugScene.cpp
 * @brief インゲーム（ステージを遊ぶシーン）
 */
#include "stdafx.h"
#include "DebugScene.h"

#include "imgui.h"
#include "ResultScene.h"
#include "Source/Actor/Character/CharacterStateMachine.h"
#include "Source/Actor/Character/CharacterStatus.h"
#include "Source/Core/DebugUI.h"
#include "Source/Sound/SoundManager.h"
#include "Source/Stage/StageCatalog.h"
#include "Source/Stage/StageLoader.h"
#include <cmath>


namespace
{
	/** プレイヤーの開始位置（足元。JSON の "playerStart" を読むまでの仮の値） */
	const app::math::Vec2 PLAYER_START_POSITION = { 230.0f, 0.0f };
	/** これより下に落ちたら死亡 */
	constexpr float KILL_Y = -1000.0f;
	/** 死んでからリスタートするまでの時間 */
	constexpr float RESTART_DELAY = 2.0f;

	/** 霧のエフェクトの番号（EffectEngine::ResistEffect と EffectEmitter::Init で同じ番号を使う） */
	constexpr int MIST_EFFECT_NUMBER = 0;
	/** 霧のエフェクトのファイル（テクスチャは Assets/effect/Texture/Smoke.png を読む） */
	const char16_t* const MIST_EFFECT_PATH = u"Assets/effect/Mist.efk";
	/** 氷が溶けきったときに出す湯気の大きさ（霧の煙に対する割合） */
	constexpr float ICE_STEAM_PUFF_RATE = 0.6f;

	/** ゴールしてからリザルトへ移り始めるまでの時間 */
	constexpr float CLEAR_WAIT_TIME = 1.2f;
	/** リザルトへ移るときのフェードの時間 */
	constexpr float CLEAR_FADE_TIME = 1.0f;

}


namespace app
{
	DebugScene::DebugScene()
		: m_inputDevice(0)
		, m_playerStartPosition(PLAYER_START_POSITION)
		, m_respawnPosition(PLAYER_START_POSITION)
		, m_stageMessages()
		, m_respawnName()
		, m_stageBodies()
		, m_isMistEffectEnabled(true)
		, m_mistPuffScale(40.0f)
		, m_mistPuffInterval(0.25f)
		, m_mistPuffBand(250.0f)
		, m_mistPuffDepth(100.0f)
		, m_mistFillRate(0.6f)
		, m_dewPuffScaleRate(0.4f)
		, m_mistPuffTimers()
		, m_ventPositions()
		, m_random(12345u)
		, m_mistPuffCount(0)
		, m_gameCamera()
		, m_isAutoRestart(true)
		, m_deadTimer(0.0f)
		, m_isShowDebugWindow(true)
		, m_isCleared(false)
		, m_clearTimer(0.0f)
		, m_warpCount(0)
	{}


	bool DebugScene::Start()
	{
		// ステージ（今のステージの JSON を読む。どのファイルかは StageCatalog.h）
		m_stageEditor.SetStageInfo(&app::stage::GetCurrentStageInfo());
		LoadStage();

		// BGM
		SoundManager::Get().PlayBGM(enSoundKind_InGame);

		// 霧のエフェクトを読み込んでおく（EffectEmitter::Init より先に必要）
		EffectEngine::GetInstance()->ResistEffect(MIST_EFFECT_NUMBER, MIST_EFFECT_PATH);

		// 地形の描画（今は黄色い床をブロックの上面に置く）
		app::stage::StageRenderer::Config rendererConfig;
		rendererConfig.modelPath = "Assets/modelData/Box.tkm";
		rendererConfig.upAxis = enModelUpAxisZ;
		rendererConfig.modelSize = Vector3(100.0f, 100.0f, 100.0f);
		rendererConfig.isPlane = false;
		rendererConfig.isPivotAtBottom = true;
		m_stageRenderer.Init(rendererConfig);

		// 光（半透明。Assets/shader/lightBeam.fx）
		m_lightBeamRenderer.Init(app::render::LightBeamRenderer::Config());

		// ギミックの描画（Assets/modelData/gimmick/ の仮モデル）
		m_gimmickRenderer.Init(app::stage::GimmickRenderer::Config());
		// 鏡の境目の半透明ガラス（Assets/shader/mirrorGlass.fx）
		m_mirrorGlass.Init();
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

		// SE：今の状態を覚えておく（ここからの変化で鳴らす）
		m_sePlayer.Reset(m_stage, m_player);

		// カメラ（投影の設定。ズームや近平面・遠平面などの値は CameraParameter.h）
		m_gameCamera.Init(g_camera3D);
		// 最初のフレームからプレイヤーを映す（原点からすべってこないように）
		UpdateCamera(0.0f, true);

		return true;
	}


	void DebugScene::Update()
	{
		const float deltaTime = g_gameTime->GetFrameDeltaTime();

		// デバッグ用の表示が OFF の間は、F1 / F2 / F5 も ImGui のウィンドウも使わない（見えないウィンドウを触らないように）
		const bool isDebugUI = app::debug::IsDebugUIVisible();
		if (isDebugUI)
		{
			UpdateHotkeys();
		}
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
		// 日よけ板と反射鏡は同じボタン（RB1）でつかむので、同時につかまないようにする
		// ・反射鏡を引きずっている間は、日よけ板をつかまない
		// ・日よけ板を引きずっている間は、反射鏡を操作しない（Reset で「回した」などの印も消しておく）
		m_shadeInteractor.Update(m_stage, m_player, !m_mirrorInteractor.IsDragging());
		if (m_shadeInteractor.IsDragging())
		{
			m_mirrorInteractor.Reset();
		}
		else
		{
			m_mirrorInteractor.Update(m_stage, m_player);
		}

		UpdateWarpAndGoal();
		UpdateCheckpoint();
		UpdateStageEvents(deltaTime);
		// SE：ステージ・プレイヤー・鏡の操作が全部終わってから、変化を見て鳴らす
		m_sePlayer.Update(deltaTime, m_stage, m_player, m_mirrorInteractor);
		if (m_isCleared)
		{
			m_clearTimer += deltaTime;
		}
		UpdateRestart(deltaTime);

		m_stageRenderer.Update(m_stage);
		m_gimmickRenderer.Update(m_stage);
		m_lightBeamRenderer.Update(m_stage);
		UpdateCamera(deltaTime, false);
		// ガラスはカメラの位置を使うので、カメラを動かしたあとに更新する
		m_mirrorGlass.Update(m_stage.GetBackMirror(), deltaTime);
		SceneLight::GetInstance()->Update();

		if (isDebugUI)
		{
			DrawImGui();
		}
	}


	void DebugScene::Render(RenderContext& rc)
	{
		m_stageRenderer.Draw(rc);
		m_gimmickRenderer.Draw(rc);
		m_player.Render(rc);
		// 半透明のガラスは、ほかのモデルを全部登録したあとに登録する（エンジンは登録した順に描く）
		m_mirrorGlass.Draw(rc);
		// 光（半透明。最後に登録する）
		m_lightBeamRenderer.Draw(rc);
	}


	bool DebugScene::RequestScene(uint32_t& id, float& fadeTime)
	{
		if (m_isCleared && m_clearTimer >= CLEAR_WAIT_TIME)
		{
			fadeTime = CLEAR_FADE_TIME;
			const int nextStage = app::stage::GetCurrentStageIndex() + 1;
			if (nextStage < app::stage::STAGE_COUNT)
			{
				// 次のステージがある：インゲームを作り直す（新しい DebugScene の Start が次のステージの JSON を読む）
				app::stage::SetCurrentStageIndex(nextStage);
				id = DebugScene::ID();
			}
			else
			{
				// 最後のステージ：ゲームクリア（リザルトへ）
				id = ResultScene::ID();
				SoundManager::Get().FadeOutBGM(CLEAR_FADE_TIME);
			}
			return true;
		}
		return false;
	}


	bool DebugScene::LoadStage()
	{
		m_stageMessages.clear();
		const bool isLoaded = app::stage::LoadStageFromJson(m_stage, app::stage::GetCurrentStageInfo(), m_stageMessages);
		// 読めなかった理由や、飛ばしたギミックを出力ウィンドウにも出す
		for (const std::string& message : m_stageMessages)
		{
			OutputDebugStringA(("[Stage] " + message + "\n").c_str());
		}
		if (isLoaded)
		{
			m_playerStartPosition = m_stage.GetPlayerStart();
			m_respawnPosition = m_playerStartPosition;
		}
		return isLoaded;
	}


	void DebugScene::ReloadStage()
	{
		// 読めなかったら（文法の間違いなど）今のステージのまま
		if (!LoadStage())
		{
			return;
		}
		// 作り直したらチェックポイントも最初から
		m_respawnPosition = m_playerStartPosition;
		m_respawnName.clear();
		RestartPlayers();
	}


	void DebugScene::ChangeStage(const int index)
	{
		app::stage::SetCurrentStageIndex(index);
		// エディタの保存先も切り替える
		m_stageEditor.SetStageInfo(&app::stage::GetCurrentStageInfo());
		ReloadStage();
	}


	void DebugScene::RestartPlayers()
	{
		m_stage.ResetRuntimeState();
		m_mirrorInteractor.Reset();
		m_shadeInteractor.Reset();

		m_player.SetPosition(m_respawnPosition);
		m_player.SetFacing(1);

		// コントローラー・ステータス・ステートマシンを作り直す（モデルはそのまま）
		m_player.SetupCharacter();
		// 死亡ステートで止めた入力を戻す
		m_player.GetPlayerController().SetEnabled(true);

		m_deadTimer = 0.0f;
		m_isCleared = false;
		m_clearTimer = 0.0f;
		UpdateCamera(0.0f, true);

		// SE：仕掛けを戻したぶんの音（扉が閉じた・受光器が消えた）は鳴らさずに覚え直して、再開の音だけ鳴らす
		m_sePlayer.Reset(m_stage, m_player);
		m_sePlayer.Play(enSoundKind_PlayerRespawn);
	}


	void DebugScene::UpdateWarpAndGoal()
	{
		if (m_player.GetStatus()->IsDead() || m_isCleared)
		{
			return;
		}
		const app::math::Rect body = m_player.GetController().GetBounds();

		// ワープ：体の中心が入ったら飛ばす（はずれの通路は入口へ、正解の通路はゴールの部屋へ）
		if (const app::stage::WarpZone* warp = m_stage.FindWarpZone(body))
		{
			TeleportPlayer(warp->destination);
			++m_warpCount;
			m_sePlayer.Play(enSoundKind_Warp);
			return;
		}

		// ゴール：触れたらクリア（操作を止めて表示を出す。F5 か Reload でやり直し）
		if (m_stage.FindGoal(body) != nullptr)
		{
			m_isCleared = true;
			m_clearTimer = 0.0f;
			m_player.GetPlayerController().SetEnabled(false);
			// 鏡を引きずっていたら音を止めて、ゴールの音を鳴らす
			m_sePlayer.StopLoops();
			m_sePlayer.Play(enSoundKind_Goal);
		}
	}


	void DebugScene::TeleportPlayer(const app::math::Vec2& destination)
	{
		// 体の位置（コントローラー）と見た目の位置の両方を動かす。速度はそのままなので走り続ける
		m_player.GetController().SetPosition(destination);
		m_player.SetPosition(destination);
		// 鏡や日よけ板を引きずっていたら離す
		m_mirrorInteractor.Reset();
		m_shadeInteractor.Reset();
		UpdateCamera(0.0f, true);
	}


	void DebugScene::UpdateCheckpoint()
	{
		// 死んでいる間は更新しない（落ちていく途中でチェックポイントを通っても再開位置にしない）
		if (m_player.GetStatus()->IsDead())
		{
			return;
		}
		if (const app::stage::Checkpoint* checkpoint = m_stage.FindCheckpoint(m_player.GetController().GetBounds()))
		{
			// 新しいチェックポイントに触れたときだけ鳴らす（同じところに立っている間は鳴らさない）
			if (checkpoint->name != m_respawnName)
			{
				m_sePlayer.Play(enSoundKind_Checkpoint);
			}
			m_respawnPosition = checkpoint->GetSpawnPosition();
			m_respawnName = checkpoint->name;
		}
	}


	void DebugScene::UpdateStageEvents(const float deltaTime)
	{
		// ---- 氷が溶けきった瞬間：湯気（霧の煙を小さくした物）を出す ----
		for (const app::stage::IceBlock& ice : m_stage.GetGimmicks().View<app::stage::IceBlock>())
		{
			if (ice.isMeltedThisFrame)
			{
				const app::math::Vec2 bottom((ice.rect.min.x + ice.rect.max.x) * 0.5f, ice.rect.min.y);
				PlayMistPuff(Vector3(bottom.x, bottom.y + 20.0f, 0.0f), m_mistPuffScale * ICE_STEAM_PUFF_RATE);
			}
		}

		if (!m_isMistEffectEnabled)
		{
			return;
		}

		// ---- 霧：吹いている間、吹き出し口ごとに煙を出し続ける ----
		// Mist.efk は「横に広がりながら薄れていく煙」（約2秒で消える）なので、
		// 霧の先端あたりに少しずつ重ねて出すと、霧の層が天井から下りてくるように見える
		const auto mists = m_stage.GetGimmicks().View<app::stage::MistEmitter>();
		m_mistPuffTimers.resize(mists.size(), 0.0f);
		size_t i = 0;
		for (const app::stage::MistEmitter& mist : mists)
		{
			const size_t index = i++;
			// 吹き出した瞬間にすぐ出す
			if (mist.isFiredThisFrame)
			{
				m_mistPuffTimers[index] = 0.0f;
			}
			// 吹いている間だけ出す（吹き終わったら、出ている煙が自然に消えて霧が晴れる）
			if (!mist.hasFired || mist.timer > mist.duration)
			{
				continue;
			}
			m_mistPuffTimers[index] -= deltaTime;
			if (m_mistPuffTimers[index] > 0.0f)
			{
				continue;
			}
			m_mistPuffTimers[index] += app::math::Max(m_mistPuffInterval, 0.02f);

			const float frontY = mist.GetFrontY();
			m_stage.GetMistVentPositions(mist, m_ventPositions);
			const float pitch = m_ventPositions.size() > 1 ? m_ventPositions[1].x - m_ventPositions[0].x : mist.rect.Width();
			for (const app::math::Vec2& vent : m_ventPositions)
			{
				// ① 霧の先端：先端から上へ band の間に出す（下りてくる霧の層）。横は吹き出し口の間隔の半分までずらす
				{
					const float top = app::math::Min(vent.y, frontY + m_mistPuffBand);
					const float y = frontY + (top - frontY) * Random01();
					const float x = vent.x + RandomSigned() * pitch * 0.5f;
					PlayMistPuff(Vector3(x, y, RandomSigned() * m_mistPuffDepth), m_mistPuffScale);
				}
				// ② 霧が通ったあと：先端より上（天井まで）のどこかに出す（部屋に霧がたまっていく）
				if (Random01() < m_mistFillRate && vent.y - frontY > m_mistPuffBand)
				{
					const float y = frontY + m_mistPuffBand + (vent.y - frontY - m_mistPuffBand) * Random01();
					const float x = mist.rect.min.x + mist.rect.Width() * Random01();
					PlayMistPuff(Vector3(x, y, RandomSigned() * m_mistPuffDepth), m_mistPuffScale);
				}
			}
		}

		// ---- 透明の足場が見えた瞬間：足場に霧がまとわりつく小さな煙を出す ----
		for (const app::stage::InvisibleFloor& platform : m_stage.GetGimmicks().View<app::stage::InvisibleFloor>())
		{
			if (!platform.isRevealedThisFrame)
			{
				continue;
			}
			const app::math::Vec2 center((platform.rect.min.x + platform.rect.max.x) * 0.5f, platform.rect.max.y);
			PlayMistPuff(Vector3(center.x, center.y, 0.0f), m_mistPuffScale * m_dewPuffScaleRate);
		}
	}


	void DebugScene::PlayMistPuff(const Vector3& position, const float scale)
	{
		// EffectEmitter は再生が終わると自分で DeleteGO される
		auto play = [&](const Vector3& at)
			{
				EffectEmitter* effect = NewGO<EffectEmitter>(0);
				effect->Init(MIST_EFFECT_NUMBER);
				effect->SetPosition(at);
				effect->SetScale(Vector3(scale, scale, scale));
				effect->Play();
				// 再生した瞬間に位置と大きさを反映する（しないと最初の1フレームだけ原点に出る）
				effect->Update();
				++m_mistPuffCount;
			};
		play(position);

		// 奥の鏡に映った煙（煙は板ポリゴンがカメラを向くので、位置を鏡の奥に移すだけでよい）
		const app::stage::BackMirror& backMirror = m_stage.GetBackMirror();
		if (backMirror.isEnabled && m_gimmickRenderer.GetConfig().isDrawReflection)
		{
			play(Vector3(position.x, position.y, backMirror.ReflectZ(position.z)));
		}
	}


	float DebugScene::RandomSigned()
	{
		std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
		return distribution(m_random);
	}


	float DebugScene::Random01()
	{
		std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
		return distribution(m_random);
	}


	void DebugScene::UpdateRestart(const float deltaTime)
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


	void DebugScene::UpdateCamera(const float deltaTime, const bool isImmediate)
	{
		// プレイヤーの足元を「見たい位置」として渡す（注視点の高さ・なめらかさ・投影は GameCamera が決める）
		const app::math::Vec2& playerPos = m_player.GetPosition();
		const Vector3 followPosition(playerPos.x, playerPos.y, 0.0f);
		if (isImmediate)
		{
			// リスタート・ワープの直後はすぐ追いつく
			m_gameCamera.SnapTo(followPosition);
		}
		else
		{
			m_gameCamera.Update(deltaTime, followPosition);
		}
	}


	void DebugScene::ApplyTuning()
	{
		// 重力などコントローラーに渡してある値は毎フレーム反映し直す（体の大きさはリスタートで反映）
		const app::actor::PlayerParameter& param = m_player.GetPlayerParameter();
		m_player.GetController().SetGravity(param.gravity);
		m_player.GetController().SetMaxFallSpeed(param.maxFallSpeed);
	}


	void DebugScene::UpdateHotkeys()
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


	void DebugScene::DrawImGui()
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


	void DebugScene::DrawDebugWindow()
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
		// ---- ステージ ----
		ImGui::SeparatorText("Stage");
		const int currentStage = app::stage::GetCurrentStageIndex();
		ImGui::Text("%s  (%d / %d)", app::stage::STAGE_INFO_TABLE[currentStage].name, currentStage + 1, app::stage::STAGE_COUNT);
		// ステージの切り替え（StageCatalog.h の表から選ぶ）
		if (ImGui::BeginCombo("Stage", app::stage::STAGE_INFO_TABLE[currentStage].name))
		{
			for (int i = 0; i < app::stage::STAGE_COUNT; ++i)
			{
				if (ImGui::Selectable(app::stage::STAGE_INFO_TABLE[i].name, i == currentStage) && i != currentStage)
				{
					ChangeStage(i);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SetItemTooltip("Load another stage from its JSON files. Unsaved editor changes are lost.");
		// ゴールに触れたのと同じ扱いにする（次のステージへ進む流れ・ゲームクリアの確認用）
		ImGui::BeginDisabled(m_isCleared);
		if (ImGui::Button("Clear This Stage"))
		{
			m_isCleared = true;
			m_clearTimer = 0.0f;
			m_player.GetPlayerController().SetEnabled(false);
		}
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Same as touching the goal: next stage, or the result screen after the last stage.");
		if (ImGui::Button("Reload JSON"))
		{
			ReloadStage();
		}
		ImGui::SetItemTooltip("Rebuild the stage from the JSON files (%s). Unsaved editor changes are lost.", app::stage::GetCurrentStageInfo().name);
		// 読み込みのメッセージ（失敗の理由・飛ばしたギミック）
		for (const std::string& message : m_stageMessages)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "%s", message.c_str());
		}
		ImGui::Checkbox("Auto restart on death", &m_isAutoRestart);
		ImGui::Text("Respawn: %s (%.0f, %.0f)", m_respawnName.empty() ? "Start" : m_respawnName.c_str(), m_respawnPosition.x, m_respawnPosition.y);
		ImGui::Text("Warps: %d   %s", m_warpCount, m_isCleared ? "CLEARED!" : "");

		// チェックポイントへワープ（部屋2などを最初から確かめたいとき）
		if (!m_stage.GetGimmicks().View<app::stage::Checkpoint>().empty())
		{
			ImGui::TextUnformatted("Warp:");
			if (ImGui::SmallButton("Start"))
			{
				m_respawnPosition = m_playerStartPosition;
				m_respawnName.clear();
				RestartPlayers();
			}
			for (const app::stage::Checkpoint& checkpoint : m_stage.GetGimmicks().View<app::stage::Checkpoint>())
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
			m_gameCamera.DrawSettingsUI();
		}

		// ---- SE ----
		if (ImGui::CollapsingHeader("Sound"))
		{
			ImGui::Checkbox("Gameplay SE", &m_sePlayer.isEnabled);
			ImGui::DragFloat("Footstep Interval", &m_sePlayer.footstepInterval, 0.01f, 0.05f, 2.0f, "%.2f s");
			ImGui::DragFloat("First Footstep Delay", &m_sePlayer.firstFootstepDelay, 0.01f, 0.0f, 1.0f, "%.2f s");
			ImGui::DragFloat("Min Footstep Speed", &m_sePlayer.minFootstepSpeed, 1.0f, 0.0f, 2000.0f, "%.0f");
			ImGui::DragFloat("Min Air Time (Land)", &m_sePlayer.minAirTimeForLand, 0.01f, 0.0f, 2.0f, "%.2f s");
			ImGui::SetItemTooltip("No landing sound after a shorter time in the air (stepping down a small ledge).");
			ImGui::DragFloat("Slide Stop Delay", &m_sePlayer.slideStopDelay, 0.01f, 0.0f, 1.0f, "%.2f s");
			const int lastKind = m_sePlayer.GetLastPlayedKind();
			const char* lastPath = GetSoundAssetPath(lastKind);
			ImGui::Text("Played: %d", m_sePlayer.GetPlayCount());
			ImGui::Text("Last: %s", lastPath != nullptr ? lastPath : "-");
			if (lastPath != nullptr && !SoundManager::Get().IsLoaded(lastKind))
			{
				ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "(file not found - silent)");
			}
		}

		// ---- 表示 ----
		if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
		{
			m_overlay.DrawSettingsUI();
			if (ImGui::TreeNode("Mist Effect"))
			{
				ImGui::Checkbox("Enabled", &m_isMistEffectEnabled);
				ImGui::DragFloat("Puff Scale", &m_mistPuffScale, 0.5f, 1.0f, 500.0f, "%.1f");
				ImGui::DragFloat("Puff Interval", &m_mistPuffInterval, 0.01f, 0.02f, 2.0f, "%.2f s");
				ImGui::DragFloat("Puff Band", &m_mistPuffBand, 5.0f, 0.0f, 2000.0f, "%.0f");
				ImGui::SetItemTooltip("Puffs appear between the mist front and this height above it.");
				ImGui::DragFloat("Puff Depth", &m_mistPuffDepth, 5.0f, 0.0f, 1000.0f, "%.0f");
				ImGui::SliderFloat("Fill Rate", &m_mistFillRate, 0.0f, 1.0f, "%.2f");
				ImGui::SetItemTooltip("Chance of an extra puff above the mist front (so the mist stays in the room).");
				ImGui::DragFloat("Reveal Puff Rate", &m_dewPuffScaleRate, 0.01f, 0.0f, 3.0f, "%.2f");
				ImGui::Text("Puffs played: %d", m_mistPuffCount);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Mirror Glass"))
			{
				app::render::MirrorGlass::Config& glass = m_mirrorGlass.GetConfig();
				ImGui::Checkbox("Visible", &glass.isVisible);
				ImGui::SameLine();
				ImGui::TextDisabled(m_mirrorGlass.IsDrawn() ? "(drawn)" : "(hidden)");
				ImGui::SliderFloat("Opacity", &glass.opacity, 0.0f, 3.0f, "%.2f");
				ImGui::ColorEdit4("Tint", glass.tint);
				ImGui::ColorEdit3("Edge Color", glass.edgeColor, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Edge Width", &glass.edgeWidth, 5.0f, 0.0f, 2000.0f, "%.0f");
				ImGui::ColorEdit3("Sheen Color", glass.sheenColor);
				ImGui::SliderFloat("Sheen Opacity", &glass.sheenOpacity, 0.0f, 1.0f, "%.2f");
				ImGui::DragFloat("Sheen Spacing", &glass.sheenSpacing, 10.0f, 0.0f, 20000.0f, "%.0f");
				ImGui::DragFloat("Sheen Speed", &glass.sheenSpeed, 0.005f, -2.0f, 2.0f, "%.3f");
				ImGui::DragFloat("Panel Size", &glass.panelSize, 10.0f, 0.0f, 10000.0f, "%.0f");
				ImGui::SetItemTooltip("Size of one mirror panel (0 = no seams).");
				ImGui::DragFloat("Seam Width", &glass.seamWidth, 0.1f, 0.0f, 100.0f, "%.1f");
				ImGui::SliderFloat("Seam Opacity", &glass.seamOpacity, 0.0f, 1.0f, "%.2f");
				ImGui::SliderFloat("Fresnel", &glass.fresnel, 0.0f, 3.0f, "%.2f");
				ImGui::SliderFloat("Top Brightness", &glass.topBrightness, 0.0f, 3.0f, "%.2f");
				ImGui::DragFloat("Z Offset", &glass.zOffset, 0.1f, -100.0f, 100.0f, "%.1f");
				ImGui::SetItemTooltip("How far in front of the Back Mirror plane the glass sits.");
				ImGui::TreePop();
			}
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
}
