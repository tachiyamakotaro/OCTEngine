#pragma once
#include "Source/Actor/Character/Player/Player.h"
#include "Source/Input/GamePadInputDevice.h"
#include "Source/Render/MirrorGlass.h"
#include "Source/Scene/IScene.h"
#include "Source/Sound/GameplaySEPlayer.h"
#include "Source/Stage/Editor/StageDebugOverlay.h"
#include "Source/Stage/Editor/StageEditor.h"
#include "Source/Stage/GimmickRenderer.h"
#include "Source/Stage/MirrorInteractor.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/StageRenderer.h"
#include <random>
#include <string>
#include <vector>


/**
 * @file DebugScene.h
 * @brief インゲーム（ステージを遊ぶシーン。デバッグ用の ImGui ウィンドウやエディタもここにある）
 */
 /**
  * @brief インゲームのシーン（ステージ1つ分のプレイ）
  * @details 持ち物
  *          ・Stage            … 地形・光・扉・光の足場・スイッチ・霧・鏡の足場・チェックポイント・奥の鏡
  *          ・Player           … プレイヤー（奥の鏡に映った姿も一緒に描く）
  *          ・MirrorInteractor … 反射鏡の回転・移動
  *          ・StageRenderer    … 地形をモデル（Box.tkm）で描く（鏡に映った床も）
  *          ・GimmickRenderer  … 扉・受光器・光の足場・光源・反射鏡・光・鏡の枠をモデルで描く
  *          ・MirrorGlass      … 本物の世界と鏡の中の世界の境目（奥の鏡の面）に置く、ステージ全体ほどの半透明のガラス
  *          ・霧のエフェクト   … 霧が吹いている間、Effekseer の煙（Assets/effect/Mist.efk）を霧の先端あたりに出し続ける
  *          ・StageDebugOverlay… 当たり判定などを画面に重ねて描く（デバッグ用）
  *          ・StageEditor      … ImGui のステージエディタ（F1 で開閉）
  *          ・GameplaySEPlayer … プレイヤーの動き・ダメージ・ギミックの SE（ワープ・ゴールなどはこのシーンから鳴らす）
  *
  *          ゴールに触れて少し待つと、リザルト（クリア画面）へ移る。
  *
  *          プレイヤーは NewGO せずシーンが直接持ち、更新の順番をシーンが決めている。
  *          （ステージ → プレイヤー → 鏡の操作 の順にしないと、光や扉が1フレームずれるため）
  */
namespace app
{
	class InGameSceneScene : public IScene
	{
		appScene(DebugScene);

	public:
		InGameSceneScene();
		~InGameSceneScene() override = default;

		bool Start() override;
		void Update() override;
		void Render(RenderContext& rc) override;
		/** ゴールして少し待ったらリザルトへ */
		bool RequestScene(uint32_t& id, float& fadeTime) override;
		/** ポーズできる（ゴールしたあとはできない） */
		bool CanPause() const override { return !m_isCleared; }


	private:
		/** ステージをコードの定義から作り直して、プレイヤーを最初の位置に戻す */
		void ReloadStage();
		/** ステージはそのままで、プレイヤーと扉・光・霧などの状態だけ最初に戻す（最後に触れたチェックポイントから再開） */
		void RestartPlayers();
		/** プレイヤーが触れたチェックポイントを再開位置にする */
		void UpdateCheckpoint();
		/** ワープに入ったら飛ばす。ゴールに触れたらクリアにする */
		void UpdateWarpAndGoal();
		/** プレイヤーを別の場所へ動かす（走る勢いはそのまま。カメラはすぐに追いつく） */
		void TeleportPlayer(const app::math::Vec2& destination);
		/** ステージで起きた出来事（霧が吹いている・鏡の足場が見えた）に合わせてエフェクトを出す */
		void UpdateStageEvents(float deltaTime);
		/**
		 * @brief 霧の煙を1つ出す（奥の鏡があれば鏡に映った煙も出す）
		 * @param position 位置（XY平面 + 奥行き z）
		 * @param scale 大きさ
		 */
		void PlayMistPuff(const Vector3& position, float scale);
		/** -1〜1 の乱数 */
		float RandomSigned();
		/** 0〜1 の乱数 */
		float Random01();
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
		/** 鏡の境目の半透明ガラス */
		app::render::MirrorGlass m_mirrorGlass;
		/** デバッグ表示 */
		app::stage::StageDebugOverlay m_overlay;
		/** ステージエディタ */
		app::stage::StageEditor m_stageEditor;
		/** インゲームの SE */
		app::GameplaySEPlayer m_sePlayer;

		/** プレイヤーの開始位置 */
		app::math::Vec2 m_playerStartPosition;
		/** 死んだとき・リスタートしたときに再開する位置（チェックポイントに触れると変わる） */
		app::math::Vec2 m_respawnPosition;
		/** 最後に触れたチェックポイントの名前（表示用。なければ空） */
		std::string m_respawnName;
		/** スイッチを踏める体（毎フレーム使い回す入れ物） */
		std::vector<app::math::Rect> m_stageBodies;

		// ---- 霧のエフェクト ----
		/** 霧のエフェクトを出すか */
		bool m_isMistEffectEnabled;
		/** 煙1つの大きさ（Mist.efk は大きさ1だと幅10ほどの煙なので、部屋に合わせて大きくする） */
		float m_mistPuffScale;
		/** 煙を出す間隔（秒。吹き出し口ごとにこの間隔で出す） */
		float m_mistPuffInterval;
		/** 煙を出す高さの幅（霧の先端からこの高さまでの間に出す） */
		float m_mistPuffBand;
		/** 煙を出す奥行きのばらつき（±） */
		float m_mistPuffDepth;
		/** 霧が通ったあと（先端より上）にも煙を出す割合（0〜1。霧が部屋にたまって見える） */
		float m_mistFillRate;
		/** 鏡の足場が見えたときに出す煙の大きさ（霧の煙に対する割合） */
		float m_dewPuffScaleRate;
		/** 霧の装置ごとの、次に煙を出すまでの時間 */
		std::vector<float> m_mistPuffTimers;
		/** 霧の吹き出し口の位置（使い回す入れ物） */
		std::vector<app::math::Vec2> m_ventPositions;
		/** 煙の位置をばらつかせる乱数 */
		std::mt19937 m_random;
		/** 出した煙の数（確認用） */
		int m_mistPuffCount;

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
		/** ゴールしたか */
		bool m_isCleared;
		/** ゴールしてからの時間（クリアの表示の演出用） */
		float m_clearTimer;
		/** ワープした回数（確認用） */
		int m_warpCount;
	};
}
