#pragma once
#include <string>
#include <vector>
#include "Source/Actor/Character/Player/Player.h"
#include "Source/Input/GamePadInputDevice.h"
#include "Source/Stage/Editor/StageDebugOverlay.h"
#include "Source/Stage/Editor/StageEditor.h"
#include "Source/Stage/GimmickRenderer.h"
#include "Source/Stage/MirrorInteractor.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/StageRenderer.h"


/**
 * @brief ゲーム本体（ステージ1つ分のプレイ）
 * @details 持ち物
 *          ・Stage            … 地形・光・扉・光の足場・スイッチ・霧・鏡の足場・チェックポイント・奥の鏡
 *          ・Player           … プレイヤー（奥の鏡に映った姿も一緒に描く）
 *          ・MirrorInteractor … 反射鏡の回転・移動
 *          ・StageRenderer    … 地形をモデル（Box.tkm）で描く（鏡に映った床も）
 *          ・GimmickRenderer  … 扉・受光器・光の足場・光源・反射鏡・光・鏡の枠をモデルで描く
 *          ・StageDebugOverlay… 当たり判定などを画面に重ねて描く（デバッグ用）
 *          ・StageEditor      … ImGui のステージエディタ（F1 で開閉）
 *
 *          プレイヤーは NewGO せず Game が直接持ち、更新の順番を Game が決めている。
 *          （ステージ → プレイヤー → 鏡の操作 の順にしないと、光や扉が1フレームずれるため）
 */
class Game : public IGameObject
{
public:
	Game();
	~Game() override = default;

	bool Start() override;
	void Update() override;
	void Render(RenderContext& rc) override;


private:
	/** ステージをコードの定義から作り直して、プレイヤーを最初の位置に戻す */
	void ReloadStage();
	/** ステージはそのままで、プレイヤーと扉・光・霧などの状態だけ最初に戻す（最後に触れたチェックポイントから再開） */
	void RestartPlayers();
	/** プレイヤーが触れたチェックポイントを再開位置にする */
	void UpdateCheckpoint();
	/** ステージで起きた出来事（霧が吹き出した・鏡の足場が見えた）に合わせてエフェクトや音を出す */
	void UpdateStageEvents();
	/** 死亡・落下を見てリスタートする */
	void UpdateRestart(float deltaTime);
	/** カメラを動かす */
	void UpdateCamera(float deltaTime, bool isImmediate);
	/** ImGui で調整したパラメーターをプレイヤーに反映する */
	void ApplyTuning();
	/** F1 / F2 などのキー操作 */
	void UpdateHotkeys();
	/** ImGui の描画 */
	void DrawImGui();
	/** デバッグウィンドウ */
	void DrawDebugWindow();


private:
	/** 入力 */
	app::input::GamePadInputDevice m_inputDevice;
	/** ステージ */
	app::stage::Stage m_stage;
	/** プレイヤー */
	app::actor::Player m_player;
	/** 反射鏡の操作 */
	app::stage::MirrorInteractor m_mirrorInteractor;
	/** 地形の描画 */
	app::stage::StageRenderer m_stageRenderer;
	/** ギミックの描画 */
	app::stage::GimmickRenderer m_gimmickRenderer;
	/** デバッグ表示 */
	app::stage::StageDebugOverlay m_overlay;
	/** ステージエディタ */
	app::stage::StageEditor m_stageEditor;

	/** プレイヤーの開始位置 */
	app::math::Vec2 m_playerStartPosition;
	/** 死んだとき・リスタートしたときに再開する位置（チェックポイントに触れると変わる） */
	app::math::Vec2 m_respawnPosition;
	/** 最後に触れたチェックポイントの名前（表示用。なければ空） */
	std::string m_respawnName;
	/** スイッチを踏める体（毎フレーム使い回す入れ物） */
	std::vector<app::math::Rect> m_stageBodies;

	/** 今のカメラの注視点 */
	Vector3 m_cameraTarget;
	/** 今のカメラの距離 */
	float m_cameraDistance;
	/** カメラの距離 */
	float m_followDistance;
	/** 注視点から見たカメラの高さ（少し上から見下ろすと床の上面が見える） */
	float m_cameraHeight;
	/** カメラの追従の速さ */
	float m_cameraSmoothing;

	/** 死んだら自動でリスタートするか */
	bool m_isAutoRestart;
	/** 死んでからの時間 */
	float m_deadTimer;
	/** デバッグウィンドウを表示するか */
	bool m_isShowDebugWindow;
};
