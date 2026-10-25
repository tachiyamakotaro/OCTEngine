#pragma once
#include "Source/Actor/Character/Player/Player.h"
#include "Source/Camera/GameCamera.h"
#include "Source/Input/GamePadInputDevice.h"
#include "Source/Render/DarknessRenderer.h"
#include "Source/Render/LightBeamRenderer.h"
#include "Source/Render/MirrorGlass.h"
#include "Source/Scene/IScene.h"
#include "Source/Sound/GameplaySEPlayer.h"
#include "Source/Stage/Editor/StageDebugOverlay.h"
#include "Source/Stage/Editor/StageEditor.h"
#include "Source/Stage/ColorLightRenderer.h"
#include "Source/Stage/ColorGlassRenderer.h"
#include "Source/Stage/FireGimmickRenderer.h"
#include "Source/Stage/GimmickRenderer.h"
#include "Source/Stage/MirrorInteractor.h"
#include "Source/Stage/ShadeInteractor.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/StageRenderer.h"
#include "Source/Timer/StageTimer.h"
#include "Source/UI/MirrorPromptUI.h"
#include "Source/UI/TimerUI.h"
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
  *          ・GimmickRenderer  … 扉・受光器・光の足場・光源・反射鏡・鏡の枠をモデルで描く
  *          ・LightBeamRenderer… 光源から出る光を半透明で描く（Assets/shader/lightBeam.fx）
  *          ・FireGimmickRenderer… 虫眼鏡・草・ロープ・吊るされた板と、集まる光・炎を描く（煙と音は UpdateFireEffects）
  *          ・MirrorGlass      … 本物の世界と鏡の中の世界の境目（奥の鏡の面）に置く、ステージ全体ほどの半透明のガラス
  *          ・霧のエフェクト   … 霧が吹いている間、Effekseer の煙（Assets/effect/Mist.efk）を霧の先端あたりに出し続ける
  *          ・StageDebugOverlay… 当たり判定などを画面に重ねて描く（デバッグ用）
  *          ・StageEditor      … ImGui のステージエディタ（F1 で開閉）
  *          ・GameplaySEPlayer … プレイヤーの動き・ダメージ・ギミックの SE（ワープ・ゴールなどはこのシーンから鳴らす）
  *          ・StageTimer       … 時間制限（残り時間を数える。制限時間は StageCatalog.h の表）
  *          ・TimerUI          … 時間制限の表示（画面の上の真ん中。時間切れで TIME UP）
  *          ・MirrorPromptUI   … 反射鏡の近くで出すボタンの表示（回転できる鏡は B、動かせる鏡は RB）
  *
  *          時間切れになると操作を止めて TIME UP を出し、少し待ってからステージを最初からやり直す。
  *
  *          ステージは StageCatalog.h の今のステージの JSON を読む（LoadStage）。
  *          ゴールに触れて少し待つと、次のステージへ移る（シーンを作り直す）。最後のステージならリザルト（ゲームクリア）へ移る。
  *
  *          プレイヤーは NewGO せずシーンが直接持ち、更新の順番をシーンが決めている。
  *          （ステージ → プレイヤー → 鏡の操作 の順にしないと、光や扉が1フレームずれるため）
  */
namespace app
{
	class DebugScene : public IScene
	{
		appScene(DebugScene);

	public:
		DebugScene();
		~DebugScene() override = default;

		bool Start() override;
		void Update() override;
		void Render(RenderContext& rc) override;
		/** ゴールして少し待ったら次のステージへ（最後のステージならリザルトへ） */
		bool RequestScene(uint32_t& id, float& fadeTime) override;
		/** ポーズできる（ゴールしたあと・時間切れのあとはできない） */
		bool CanPause() const override { return !m_isCleared && !m_stageTimer.IsTimeUp(); }


	private:
		/** 今のステージを JSON から読む（読めたら開始位置も更新する。読めなかったら false） */
		bool LoadStage();
		/** ステージを JSON から読み直して、プレイヤーを最初の位置に戻す（読めなかったら何もしない） */
		void ReloadStage();
		/** 別のステージに切り替える（StageCatalog.h の表の番号） */
		void ChangeStage(int index);
		/**
		 * @brief プレイヤーを最後に触れたチェックポイントから再開させる
		 * @param isResetGimmicks true  … 扉・光・霧などの仕掛けも最初に戻す（F5・Warp ボタン・時間切れ）
		 *                        false … 仕掛けは今の状態のまま（落下死などで死んだとき）
		 */
		void RestartPlayers(bool isResetGimmicks = true);
		/** プレイヤーが触れたチェックポイントを再開位置にする */
		void UpdateCheckpoint();
		/** ワープに入ったら飛ばす。ゴールに触れたらクリアにする */
		void UpdateWarpAndGoal();
		/** プレイヤーを別の場所へ動かす（走る勢いはそのまま。カメラはすぐに追いつく） */
		void TeleportPlayer(const app::math::Vec2& destination);
		/**
		 * @brief デバッグ用：マウスでプレイヤーを動かす（デバッグ表示が ON のときだけ）
		 * @details ・ゲーム画面のプレイヤーを左ドラッグ … つかんで動かす（離した所から、止まった状態で落ちる）
		 *          ・Ctrl + 左クリック … クリックした所へワープ（足元がクリックした点になる）
		 *          ステージの仕掛け（扉・氷・鏡など）は今の状態のまま（チェックポイントへのワープとの違い）
		 */
		void UpdatePlayerDrag();
		/**
		 * @brief デバッグ用：画面の位置（ピクセル）から、ステージの面（z = 0）の上の位置を求める
		 * @return 求められたら true
		 */
		bool ScreenToStage(float screenX, float screenY, app::math::Vec2& outWorld) const;
		/**
		 * @brief デバッグ用：プレイヤーを foot の位置に、止まった状態で置き直す（落下の高さ・速さも消す）
		 */
		void DropPlayerAt(const app::math::Vec2& foot);
		/**
		 * @brief デバッグ用：プレイヤーをつかんでいる間のカメラ（マウスを画面の端に近づけると、その向きへカメラが動く）
		 */
		void UpdateDragCamera(float deltaTime);
		/** ステージで起きた出来事（霧が吹いている・鏡の足場が見えた）に合わせてエフェクトを出す */
		void UpdateStageEvents(float deltaTime);
		/**
		 * @brief 霧の煙を1つ出す（奥の鏡があれば鏡に映った煙も出す）
		 * @param position 位置（XY平面 + 奥行き z）
		 * @param scale 大きさ
		 */
		void PlayMistPuff(const Vector3& position, float scale);
		/** 虫眼鏡で燃やす物の煙と音（温まっている・燃えている・燃え尽きた・板が落ちた） */
		void UpdateFireEffects(float deltaTime);
		/** -1〜1 の乱数 */
		float RandomSigned();
		/** 0〜1 の乱数 */
		float Random01();
		/** 死亡・落下を見てリスタートする */
		void UpdateRestart(float deltaTime);
		/** 時間制限を進めて、時間切れなら TIME UP を出し、少し待ってステージを最初からやり直す */
		void UpdateTimer(float deltaTime);
		/** ステージの制限時間で、時間制限を最初から数え直す */
		void StartTimer();
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
		/** 日よけ板の操作 */
		app::stage::ShadeInteractor m_shadeInteractor;
		/** 地形の描画 */
		app::stage::StageRenderer m_stageRenderer;
		/** ギミックの描画 */
		app::stage::GimmickRenderer m_gimmickRenderer;
		/** 鏡の境目の半透明ガラス */
		app::render::MirrorGlass m_mirrorGlass;
		/** 光の描画（半透明） */
		app::render::LightBeamRenderer m_lightBeamRenderer;
		/** 色の光のギミック（色センサー・扉の色の印・色の付いた光源）の描画 */
		app::stage::ColorLightRenderer m_colorLightRenderer;
		/** 色ガラスの描画 */
		app::stage::ColorGlassRenderer m_colorGlassRenderer;
		/** 虫眼鏡のギミックと、集まる光・炎の描画 */
		app::stage::FireGimmickRenderer m_fireRenderer;
		/** 暗がりの描画（半透明。光の前に描く） */
		app::render::DarknessRenderer m_darknessRenderer;
		/** デバッグ表示 */
		app::stage::StageDebugOverlay m_overlay;
		/** ステージエディタ */
		app::stage::StageEditor m_stageEditor;
		/** インゲームの SE */
		app::GameplaySEPlayer m_sePlayer;
		/** 時間制限 */
		app::timer::StageTimer m_stageTimer;
		/** 時間制限の表示 */
		app::ui::TimerUI m_timerUI;
		/** 反射鏡のインタラクト表示（B で回転 / RB で移動） */
		app::ui::MirrorPromptUI m_mirrorPromptUI;

		/** プレイヤーの開始位置 */
		app::math::Vec2 m_playerStartPosition;
		/** 死んだとき・リスタートしたときに再開する位置（チェックポイントに触れると変わる） */
		app::math::Vec2 m_respawnPosition;
		/** ステージの読み込みのメッセージ（失敗の理由・飛ばしたギミック） */
		std::vector<std::string> m_stageMessages;
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
		/** 燃える物ごとの、次に煙を出すまでの時間 */
		std::vector<float> m_firePuffTimers;

		/** カメラ（追従・投影の設定・調整用の ImGui） */
		app::camera::GameCamera m_gameCamera;

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
		/** デバッグ用：マウスでプレイヤーをつかんでいるか */
		bool m_isDraggingPlayer;
		/** デバッグ用：つかんだ点から足元までのずれ（つかんだ瞬間にプレイヤーが飛ばないように） */
		app::math::Vec2 m_dragGrabOffset;
		/** デバッグ用：つかんでいる間にカメラが見る位置（画面の端にマウスを近づけると動く） */
		app::math::Vec2 m_dragCameraTarget;
		/** 時間切れになってからの時間（TIME UP を見せてからやり直す） */
		float m_timeUpTimer;
	};
}
