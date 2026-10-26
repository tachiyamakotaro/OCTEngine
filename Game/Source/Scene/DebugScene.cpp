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
#include "Source/Scene/SceneManager.h"
#include "Source/Sound/SoundManager.h"
#include "Source/Stage/StageCatalog.h"
#include "Source/Stage/StageLoader.h"
#include "Source/Timer/TimerParameter.h"
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

	/** マウスでプレイヤーをつかむとき、体の箱をどれだけ広げてつかめるようにするか */
	constexpr float PLAYER_PICK_MARGIN = 20.0f;
	/** プレイヤーをつかんでいる間、画面の端からこのピクセル以内にマウスがあると、カメラがその向きへ動く */
	constexpr float DRAG_EDGE_MARGIN = 120.0f;
	/** そのときのカメラの速さ（画面のいちばん端での速さ。端に近いほど速い） */
	constexpr float DRAG_EDGE_SPEED = 1500.0f;
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
		, m_isAutoRestart(true)
		, m_deadTimer(0.0f)
		, m_isShowDebugWindow(true)
		, m_isCleared(false)
		, m_clearTimer(0.0f)
		, m_warpCount(0)
		, m_isDraggingPlayer(false)
		, m_dragGrabOffset()
		, m_dragCameraTarget()
		, m_timeUpTimer(0.0f)
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
		// 暗がり（半透明。Assets/shader/darkness.fx）
		m_darknessRenderer.Init(app::render::DarknessRenderer::Config());

		// ギミックの描画（Assets/modelData/gimmick/ の仮モデル）
		m_gimmickRenderer.Init(app::stage::GimmickRenderer::Config());
		// 色の光のギミック（色センサー・扉の色の印・色の付いた光源）
		m_colorLightRenderer.Init(app::stage::ColorLightRenderer::Config());
		m_colorGlassRenderer.Init(app::stage::ColorGlassRenderer::Config());
		// 虫眼鏡のギミック（虫眼鏡・草・ロープ・吊るされた板）と、集まる光・炎
		m_fireRenderer.Init(app::stage::FireGimmickRenderer::Config());
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

		// カメラ（投影の設定。ズームなどの値は CameraParameter.h）
		m_gameCamera.Init(g_camera3D);
		// 最初のフレームからプレイヤーを映す（原点からすべってこないように）
		UpdateCamera(0.0f, true);

		// 時間制限（制限時間は StageCatalog.h の表）と、その表示
		m_timerUI.Init();
		StartTimer();

		// 反射鏡のインタラクト表示（画像は Assets/sprite/prompt/）
		m_mirrorPromptUI.Init();

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
		// デバッグ用：マウスでプレイヤーを動かす（プレイヤーの更新のあとに位置を上書きする）
		if (isDebugUI)
		{
			UpdatePlayerDrag();
		}
		else
		{
			m_isDraggingPlayer = false;
		}
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

		// つかんで動かしている間は、ワープ・ゴール・チェックポイントに触れても反応しない（離したあとで反応する）
		if (!m_isDraggingPlayer)
		{
			UpdateWarpAndGoal();
			UpdateCheckpoint();
		}
		UpdateStageEvents(deltaTime);
		UpdateFireEffects(deltaTime);
		// SE：ステージ・プレイヤー・鏡の操作が全部終わってから、変化を見て鳴らす
		m_sePlayer.Update(deltaTime, m_stage, m_player, m_mirrorInteractor);
		if (m_isCleared)
		{
			m_clearTimer += deltaTime;
		}
		UpdateTimer(deltaTime);
		UpdateRestart(deltaTime);

		m_stageRenderer.Update(m_stage);
		m_gimmickRenderer.Update(m_stage);
		m_lightBeamRenderer.Update(m_stage);
		m_colorLightRenderer.Update(m_stage, deltaTime);
		m_colorGlassRenderer.Update(m_stage);
		m_fireRenderer.Update(m_stage, deltaTime);
		// つかんで動かしている間は、プレイヤーを追いかけない（追いかけると、マウスの下のステージの位置がずれ続けてしまうため）
		// 代わりに、マウスを画面の端に近づけたときだけカメラを動かす
		if (m_isDraggingPlayer)
		{
			UpdateDragCamera(deltaTime);
		}
		else
		{
			UpdateCamera(deltaTime, false);
		}
		// ガラスはカメラの位置を使うので、カメラを動かしたあとに更新する
		m_mirrorGlass.Update(m_stage.GetBackMirror(), deltaTime);
		{
			// 暗がりもカメラの向きを使うので、カメラを動かしたあとに更新する（プレイヤーのまわりは少し明るくする）
			const app::math::Rect body = m_player.GetController().GetBounds();
			m_darknessRenderer.Update(m_stage, (body.min + body.max) * 0.5f, m_gameCamera.GetParameter().isOrtho);
		}
		// 反射鏡のインタラクト表示（鏡の位置を画面の位置に変えるので、鏡の操作とカメラを動かしたあとに更新する）
		// ・日よけ板が近いときは RB が板のほうに使われるので、鏡の RB は出さない
		// ・ゴールしたあと・時間切れの間・デバッグでプレイヤーをつかんでいる間は出さない
		m_mirrorPromptUI.Update(m_stage, m_mirrorInteractor, m_player,
			m_shadeInteractor.GetFocusedIndex() < 0,
			!m_isCleared && !m_stageTimer.IsTimeUp() && !m_isDraggingPlayer,
			deltaTime);
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
		m_fireRenderer.DrawOpaque(rc);
		m_colorLightRenderer.DrawOpaque(rc);
		m_player.Render(rc);
		// 半透明のガラスは、ほかのモデルを全部登録したあとに登録する（エンジンは登録した順に描く）
		m_mirrorGlass.Draw(rc);
		// 色センサーの窓・扉の色の印（半透明）
		m_colorLightRenderer.DrawTranslucent(rc);
		// 色ガラス（半透明）
		m_colorGlassRenderer.Draw(rc);
		// 暗がり（半透明。光より先に登録して、暗がりの中でも光の筋が見えるようにする）
		m_darknessRenderer.Draw(rc);
		// 光（半透明。最後に登録する）
		m_lightBeamRenderer.Draw(rc);
		// 集まる光・炎（半透明）
		m_fireRenderer.DrawTranslucent(rc);
		// 暗い部屋の手前の壁（半透明。中の光や炎も隠したいので、3D の一番最後に登録する）
		m_darknessRenderer.DrawFrontWalls(rc);
		// 反射鏡のインタラクト表示（2D の画像は 3D のあとに描かれる。時間制限の表示より下に出す）
		m_mirrorPromptUI.Render(rc);
		// 時間制限の表示（2D の画像は 3D のあとに描かれる。ポーズ画面はこの上に出る）
		m_timerUI.Render(rc);
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
		// 時間制限も最初から（ステージを切り替えたときは、そのステージの制限時間になる）
		StartTimer();
	}


	void DebugScene::ChangeStage(const int index)
	{
		app::stage::SetCurrentStageIndex(index);
		// エディタの保存先も切り替える
		m_stageEditor.SetStageInfo(&app::stage::GetCurrentStageInfo());
		ReloadStage();
	}


	void DebugScene::RestartPlayers(const bool isResetGimmicks)
	{
		// 死んだときは仕掛けを戻さない（攻略の途中の状態のまま、チェックポイントから再開）
		if (isResetGimmicks)
		{
			m_stage.ResetRuntimeState();
		}
		// 鏡・日よけ板をつかんでいたら離す（鏡の向き・板の位置はそのまま）
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
			// 時間を止める（残り時間はそのまま表示しておく）
			m_stageTimer.SetRunning(false);
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


	void DebugScene::UpdatePlayerDrag()
	{
		const ImGuiIO& io = ImGui::GetIO();
		// 死んでいる・ゴールした・時間切れの間は動かさない
		const bool canDrag = !m_player.GetStatus()->IsDead() && !m_isCleared && !m_stageTimer.IsTimeUp();
		app::math::Vec2 mouseWorld;
		const bool hasMouse = ScreenToStage(io.MousePos.x, io.MousePos.y, mouseWorld);

		if (!m_isDraggingPlayer)
		{
			// ImGui のウィンドウの上でのクリックは、ウィンドウの操作なので使わない
			if (!canDrag || !hasMouse || io.WantCaptureMouse)
			{
				return;
			}
			app::math::Rect body = m_player.GetController().GetBounds();
			body.min -= app::math::Vec2(PLAYER_PICK_MARGIN, PLAYER_PICK_MARGIN);
			body.max += app::math::Vec2(PLAYER_PICK_MARGIN, PLAYER_PICK_MARGIN);
			const bool isOverPlayer = mouseWorld.x >= body.min.x && mouseWorld.x <= body.max.x
				&& mouseWorld.y >= body.min.y && mouseWorld.y <= body.max.y;
			if (isOverPlayer && !io.KeyCtrl)
			{
				// つかめることがわかるように、手の形のカーソルにする
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}
			if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				return;
			}
			if (io.KeyCtrl)
			{
				// Ctrl + クリック：クリックした所へワープ（足元をクリックした点にする）
				DropPlayerAt(mouseWorld);
				return;
			}
			if (!isOverPlayer)
			{
				return;
			}
			// つかむ（鏡や日よけ板を引きずっていたら離す）
			m_isDraggingPlayer = true;
			m_dragGrabOffset = m_player.GetPosition() - mouseWorld;
			// カメラは今プレイヤーを見ているので、そこから動かし始める
			m_dragCameraTarget = m_player.GetPosition();
			m_mirrorInteractor.Reset();
			m_shadeInteractor.Reset();
		}

		if (!canDrag)
		{
			m_isDraggingPlayer = false;
			return;
		}
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		// つかんでいる間：マウスに合わせて動かし、落ちないように速さを 0 にしておく（壁や床の中にも置ける）
		if (hasMouse)
		{
			const app::math::Vec2 foot = mouseWorld + m_dragGrabOffset;
			m_player.GetController().SetPosition(foot);
			m_player.GetController().SetVelocity(app::math::Vec2(0.0f, 0.0f));
			m_player.SetPosition(foot);
		}
		// 離した：その場所に止まった状態で置き直す
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			m_isDraggingPlayer = false;
			DropPlayerAt(m_player.GetPosition());
		}
	}


	bool DebugScene::ScreenToStage(const float screenX, const float screenY, app::math::Vec2& outWorld) const
	{
		const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
		if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
		{
			return false;
		}
		// 画面のピクセル → -1〜1 の座標（DrawImGui の projector の逆）
		const float ndcX = screenX / displaySize.x * 2.0f - 1.0f;
		const float ndcY = 1.0f - screenY / displaySize.y * 2.0f;
		// 画面の一番手前（0）と一番奥（1）の2点をワールドに戻し、その2点を結ぶ線が z = 0 の面と交わる所を求める
		// （透視投影でも平行投影でも同じ計算でよい）
		const Matrix& inverse = g_camera3D->GetViewProjectionMatrixInv();
		Vector4 nearPoint(ndcX, ndcY, 0.0f, 1.0f);
		Vector4 farPoint(ndcX, ndcY, 1.0f, 1.0f);
		inverse.Apply(nearPoint);
		inverse.Apply(farPoint);
		if (std::abs(nearPoint.w) < 0.0001f || std::abs(farPoint.w) < 0.0001f)
		{
			return false;
		}
		const Vector3 a(nearPoint.x / nearPoint.w, nearPoint.y / nearPoint.w, nearPoint.z / nearPoint.w);
		const Vector3 b(farPoint.x / farPoint.w, farPoint.y / farPoint.w, farPoint.z / farPoint.w);
		const float dz = b.z - a.z;
		if (std::abs(dz) < 0.0001f)
		{
			// 線が z = 0 の面と平行（ふつうは起きない）
			return false;
		}
		const float t = -a.z / dz;
		outWorld = app::math::Vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
		return true;
	}


	void DebugScene::DropPlayerAt(const app::math::Vec2& foot)
	{
		// RestartPlayers と同じように、コントローラー・ステータス・ステートマシンを作り直す
		// （速さ・落ちた高さ・崖つかまりなどが残っていると、置いた直後に落下ダメージを受けることがあるため）
		// ステージの仕掛けは戻さない
		const int facing = m_player.GetFacing();
		m_player.SetPosition(foot);
		m_player.SetFacing(facing);
		m_player.SetupCharacter();
		m_player.GetPlayerController().SetEnabled(true);
		m_mirrorInteractor.Reset();
		m_shadeInteractor.Reset();
		m_deadTimer = 0.0f;
		UpdateCamera(0.0f, true);
	}


	void DebugScene::UpdateDragCamera(const float deltaTime)
	{
		const ImGuiIO& io = ImGui::GetIO();
		app::math::Vec2 scroll(0.0f, 0.0f);
		// マウスがウィンドウの外（位置がない）なら動かさない
		if (ImGui::IsMousePosValid() && DRAG_EDGE_MARGIN > 0.0f)
		{
			// 端からの近さ（0 = 端から DRAG_EDGE_MARGIN 以上離れている 〜 1 = いちばん端・端の外）
			const auto edge = [](const float distance)
				{
					return app::math::Clamp01(1.0f - distance / DRAG_EDGE_MARGIN);
				};
			const ImVec2 mouse = io.MousePos;
			const ImVec2 size = io.DisplaySize;
			scroll.x = edge(size.x - mouse.x) - edge(mouse.x);
			// 画面は下向きが +、ステージは上向きが + なので逆にする
			scroll.y = edge(mouse.y) - edge(size.y - mouse.y);
		}
		m_dragCameraTarget += scroll * (DRAG_EDGE_SPEED * deltaTime);
		m_gameCamera.Update(deltaTime, Vector3(m_dragCameraTarget.x, m_dragCameraTarget.y, 0.0f));
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

		// 時間切れのやり直しは UpdateTimer がする（ここで先にリスタートすると TIME UP の途中で動けてしまう）
		if (!m_player.GetStatus()->IsDead() || m_stageTimer.IsTimeUp())
		{
			m_deadTimer = 0.0f;
			return;
		}

		m_deadTimer += deltaTime;
		if (m_isAutoRestart && m_deadTimer >= RESTART_DELAY)
		{
			// 仕掛けは今の状態のまま（溶かした氷・回した鏡・開いた扉などは戻さない）
			RestartPlayers(false);
		}
	}


	void DebugScene::UpdateTimer(const float deltaTime)
	{
		// フェードで画面が切り替わっている間（ステージに入った直後など）は数えない
		// ゴールしたあとは止めたまま（RestartPlayers で m_isCleared が戻ったら、また数える）
		const bool isTransitioning = SceneManager::GetInstance() != nullptr && SceneManager::GetInstance()->IsTransitioning();
		m_stageTimer.SetRunning(!m_isCleared && !isTransitioning);
		// デバッグの Stop Timer で止めたら、デバッグ表示を OFF にしても止めたままにする（動かすときは Resume Timer を押す）
		m_stageTimer.Update(deltaTime);

		// 残りわずか（危険）の間は、表示の秒数が変わるたびに「ピッ」
		if (m_stageTimer.GetLevel() == app::timer::TimeLevel::Danger && m_stageTimer.IsSecondChangedThisFrame())
		{
			m_sePlayer.Play(enSoundKind_TimerTick);
		}

		// 時間切れになった瞬間：操作を止めて、鳴り続けている音を止めて、時間切れの音
		if (m_stageTimer.IsTimeUpThisFrame())
		{
			m_timeUpTimer = 0.0f;
			m_player.GetPlayerController().SetEnabled(false);
			m_mirrorInteractor.Reset();
			m_sePlayer.StopLoops();
			m_sePlayer.Play(enSoundKind_TimeUp);
		}

		// TIME UP を少し見せてから、ステージを最初からやり直す（チェックポイントも消す）
		if (m_stageTimer.IsTimeUp())
		{
			m_timeUpTimer += deltaTime;
			if (m_timeUpTimer >= app::timer::TIME_UP_WAIT)
			{
				// ステージを JSON から読み直して、攻略前の状態に戻す
				// （ResetRuntimeState だけでは、プレイヤーが回した鏡の向き・動かした日よけ板の位置が戻らないため）
				// 読めなかったとき（JSON の書き間違いなど）は今のステージのままで、下の RestartPlayers が仕掛けだけ戻す
				LoadStage();
				m_respawnPosition = m_playerStartPosition;
				m_respawnName.clear();
				RestartPlayers();
				m_stageTimer.Restart();
				m_timeUpTimer = 0.0f;
			}
		}

		// 表示（タイマーを進めたあとに更新する）
		m_timerUI.Update(m_stageTimer, deltaTime);
	}


	void DebugScene::StartTimer()
	{
		m_stageTimer.Start(app::stage::GetCurrentStageTimeLimit());
		m_timeUpTimer = 0.0f;
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
		ImGui::TextUnformatted("Mouse: drag the player (screen edge = scroll) / Ctrl+Click: warp there");

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
			m_stageTimer.SetRunning(false);
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

		// ---- 時間制限（中身はタイマーと表示のクラスが作る） ----
		if (ImGui::CollapsingHeader("Timer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			m_stageTimer.DrawSettingsUI();
			m_timerUI.DrawSettingsUI();
		}

		// ---- 反射鏡のインタラクト表示（中身は表示のクラスが作る） ----
		if (ImGui::CollapsingHeader("Mirror Prompt"))
		{
			m_mirrorPromptUI.DrawSettingsUI();
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
			ImGui::DragFloat("Hang Model Forward", &param.ledgeHangModelForward, 0.5f, -200.0f, 200.0f, "%.1f");
			ImGui::DragFloat("Hang Model Y", &param.ledgeHangModelY, 0.5f, -200.0f, 200.0f, "%.1f");
			ImGui::SetItemTooltip("Shift the model while hanging so the hands are on the ledge corner.");
			ImGui::DragFloat("Climb Model Forward", &param.climbModelForward, 0.5f, -200.0f, 200.0f, "%.1f");
			ImGui::DragFloat("Climb Model Y", &param.climbModelY, 0.5f, -200.0f, 200.0f, "%.1f");
			ImGui::SetItemTooltip("Added to the hang shift while climbing. Match the first frame of the climb to the hang pose.");
			ImGui::DragFloat("Hard Landing Height", &param.hardLandingHeight, 5.0f, 0.0f, 3000.0f, "%.0f");
			ImGui::DragFloat("Deadly Fall Height", &param.deadlyFallHeight, 5.0f, 0.0f, 5000.0f, "%.0f");
			ImGui::Checkbox("Auto Ledge Grab", &param.isAutoLedgeGrab);
			ImGui::Separator();
			ImGui::DragFloat("Model Yaw Offset", &param.modelYawOffsetDeg, 1.0f, -180.0f, 180.0f, "%.0f deg");
			ImGui::SetItemTooltip("Set 180 if the model faces backwards.");
			ImGui::DragFloat("Model Scale", &param.modelScale, 0.01f, 0.05f, 5.0f, "%.2f");
			ImGui::SetItemTooltip("Model size only. Collision size is Body Width / Body Height.");
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


	void DebugScene::UpdateFireEffects(const float deltaTime)
	{
		// 虫眼鏡で燃やす物の煙と音。煙は霧と同じ Mist.efk を小さくして使う
		// 音は今ある SE の仮使い（火がつく = 霧の吹き出し、板が落ちた = 高い所からの着地）
		constexpr float SMOKE_START_HEAT = 0.15f;	// これより温まったら煙を出し始める
		constexpr float BURNING_PUFF_INTERVAL = 0.15f;	// 燃えている間に煙を出す間隔
		constexpr float SMOKE_Z = -20.0f;			// 煙を出す奥行き（物の少し手前）
		const float puffScale = m_mistPuffScale;

		auto puff = [&](const app::math::Vec2& at, const float scale)
			{
				if (m_isMistEffectEnabled)
				{
					PlayMistPuff(Vector3(at.x, at.y, SMOKE_Z), scale);
				}
			};

		const auto burnables = m_stage.GetGimmicks().View<app::stage::BurnableGimmick>();
		m_firePuffTimers.resize(burnables.size(), 0.0f);
		size_t index = 0;
		for (const app::stage::BurnableGimmick& burnable : burnables)
		{
			float& timer = m_firePuffTimers[index++];
			if (burnable.isIgnitedThisFrame)
			{
				m_sePlayer.Play(enSoundKind_MistBurst);
				timer = 0.0f;
			}
			if (burnable.isBurntThisFrame)
			{
				// 燃え尽きた瞬間：元の範囲に煙をまとめて出す
				for (int n = 0; n < 3; ++n)
				{
					const app::math::Rect& r = burnable.rect;
					puff({ r.min.x + r.Width() * Random01(), r.min.y + r.Height() * Random01() }, puffScale * 0.5f);
				}
				continue;
			}
			if (burnable.isBurnt || (!burnable.isBurning && burnable.heat <= SMOKE_START_HEAT))
			{
				timer = 0.0f;
				continue;
			}

			timer -= deltaTime;
			if (timer > 0.0f)
			{
				continue;
			}
			if (burnable.isBurning)
			{
				// 燃えている：今の形の上の辺から煙が立ちのぼる
				timer += BURNING_PUFF_INTERVAL;
				const app::math::Rect r = burnable.GetCurrentRect();
				puff({ r.min.x + r.Width() * Random01(), r.max.y }, puffScale * 0.35f);
			}
			else
			{
				// 温まっている：焦点から細い煙。熱いほど多く・大きく
				timer += app::math::Lerp(0.5f, 0.15f, burnable.heat);
				puff(burnable.heatPoint, puffScale * (0.1f + 0.2f * burnable.heat));
			}
		}

		// 吊るされた板が落ちて乗った瞬間：下の辺に土ぼこり
		for (const app::stage::HangingBlock& block : m_stage.GetGimmicks().View<app::stage::HangingBlock>())
		{
			if (!block.isLandedThisFrame)
			{
				continue;
			}
			m_sePlayer.Play(enSoundKind_PlayerHardLand);
			const app::math::Rect r = block.GetCurrentRect();
			for (int n = 0; n < 5; ++n)
			{
				puff({ r.min.x + r.Width() * (static_cast<float>(n) + 0.5f) / 5.0f, r.min.y }, puffScale * 0.3f);
			}
		}
	}
}
