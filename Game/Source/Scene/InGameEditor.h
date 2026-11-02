#pragma once
#include "Source/Actor/Character/Player/PlayerEditor.h"
#include "Source/Camera/CameraEditor.h"
#include "Source/Core/Math/Vec2.h"
#include "Source/Render/MirrorGlassEditor.h"
#include "Source/Render/OutlineEditor.h"
#include "Source/Sound/GameplaySEEditor.h"
#include "Source/Stage/GimmickMarkEditor.h"
#include <optional>
#include <string>
#include <vector>


/**
 * @file InGameEditor.h
 * @brief インゲームのデバッグウィンドウ「Game (F2)」（今まで DebugScene::DrawDebugWindow に書いていた ImGui をまとめた）
 */
namespace app
{
	class GameplaySEPlayer;
	struct MistEffectParameter;

	namespace actor
	{
		class Player;
	}
	namespace camera
	{
		class GameCamera;
	}
	namespace render
	{
		class MirrorGlass;
	}
	namespace stage
	{
		class Stage;
		class StageDebugOverlay;
		class StageRenderer;
		class GimmickRenderer;
		class GimmickMarkRenderer;
	}
	namespace timer
	{
		class StageTimer;
	}
	namespace ui
	{
		class TimerUI;
		class MirrorPromptUI;
	}


	/**
	 * @brief デバッグウィンドウで調整する相手（シーンが持っている物。Init で1回だけ渡す）
	 * @details シーンより長生きしない物だけを入れる（シーンのメンバーなど）
	 */
	struct InGameEditTargets
	{
		actor::Player* player = nullptr;
		stage::Stage* stage = nullptr;
		timer::StageTimer* stageTimer = nullptr;
		ui::TimerUI* timerUI = nullptr;
		ui::MirrorPromptUI* mirrorPromptUI = nullptr;
		camera::GameCamera* camera = nullptr;
		GameplaySEPlayer* sePlayer = nullptr;
		stage::StageDebugOverlay* overlay = nullptr;
		render::MirrorGlass* mirrorGlass = nullptr;
		stage::StageRenderer* stageRenderer = nullptr;
		stage::GimmickRenderer* gimmickRenderer = nullptr;
		stage::GimmickMarkRenderer* gimmickMarkRenderer = nullptr;
		MistEffectParameter* mistEffect = nullptr;
	};


	/**
	 * @brief 再開する場所（チェックポイントの名前と位置。名前が空ならステージの開始位置）
	 */
	struct RespawnPoint
	{
		/** チェックポイントの名前（開始位置なら空） */
		std::string name;
		/** 位置（足元） */
		math::Vec2 position;
	};


	/**
	 * @brief デバッグウィンドウに表示するだけの、今の状態（毎フレーム Draw に渡す）
	 */
	struct InGameStatus
	{
		/** ゴールしたか */
		bool isCleared = false;
		/** 今の再開する場所 */
		RespawnPoint respawn;
		/** ステージの開始位置（Warp の Start ボタン用） */
		math::Vec2 startPosition;
		/** ワープした回数 */
		int warpCount = 0;
		/** 出した霧の煙の数 */
		int mistPuffCount = 0;
		/** ステージの読み込みのメッセージ（失敗の理由・飛ばしたギミック。なければ nullptr） */
		const std::vector<std::string>* stageMessages = nullptr;
	};


	/**
	 * @brief デバッグウィンドウで押されたボタン（シーンにしてほしいこと）
	 * @details ウィンドウはシーンの処理を直接呼ばず、「何をしてほしいか」だけを返す。
	 *          シーンは Draw のあとに、これを見て処理する（DebugScene::ApplyEditorRequest）。
	 */
	struct InGameEditorRequest
	{
		/** ステージを切り替えない、という印 */
		static constexpr int NO_STAGE_CHANGE = -1;

		/** プレイヤーをリスタートする（Restart ボタン） */
		bool isRestart = false;
		/** 今のステージをクリアしたことにする（Clear This Stage ボタン） */
		bool isClearStage = false;
		/** JSON からステージを読み直す（Reload JSON ボタン） */
		bool isReloadStage = false;
		/** 切り替えるステージの番号（NO_STAGE_CHANGE なら切り替えない） */
		int changeStageIndex = NO_STAGE_CHANGE;
		/** 再開する場所を変えてリスタートする（Warp のボタン） */
		bool isWarp = false;
		/** そのときの再開する場所 */
		RespawnPoint warpTo;

		/** ステージを切り替えるか */
		bool IsChangeStage() const { return changeStageIndex != NO_STAGE_CHANGE; }
	};


	/**
	 * @brief インゲームのデバッグウィンドウ「Game (F2)」
	 * @details ・DebugScene に書いていた ImGui をまとめたクラス。シーンは Draw を1行呼んで、返ってきたお願いを処理するだけ
	 *          ・機能ごとの項目は、その機能のフォルダの ○○Editor が描く（Camera → CameraEditor など）。
	 *            このクラスは、それを並べることと、シーン全体にかかわる項目（ステージの切り替え・ワープ・霧の煙）だけを受け持つ
	 *          ・ウィンドウの表示・非表示（F2）と「死んだら自動でリスタート」の設定は、このクラスが持つ
	 */
	class InGameEditor
	{
	public:
		InGameEditor() = default;
		~InGameEditor() = default;

		/**
		 * @brief 調整する相手を覚える（シーンの Start で1回呼ぶ）
		 */
		void Init(const InGameEditTargets& targets);
		/**
		 * @brief デバッグウィンドウを描く（ImGui のフレームの中で、毎フレーム呼ぶ）
		 * @param status 表示する今の状態
		 * @return 押されたボタン（シーンにしてほしいこと）
		 */
		InGameEditorRequest Draw(const InGameStatus& status);

		/** ウィンドウを出す・消す（F2） */
		void ToggleWindow() { m_isOpen = !m_isOpen; }
		/** 死んだら自動でリスタートするか */
		bool IsAutoRestart() const { return m_isAutoRestart; }

	private:
		/** キー操作の説明 */
		void DrawHelp();
		/**
		 * @brief プレイヤーの状態と Restart ボタン
		 * @return Restart が押されたか
		 */
		bool DrawPlayer();
		/**
		 * @brief ステージの切り替え・クリア・読み直し・再開する場所・ワープ
		 * @return 押されたボタン
		 */
		InGameEditorRequest DrawStage(const InGameStatus& status);
		/**
		 * @brief チェックポイントへのワープのボタン
		 * @return 押されたボタンの再開する場所（押されなければ空）
		 */
		std::optional<RespawnPoint> DrawWarpButtons(const InGameStatus& status);
		/** 機能ごとの調整（○○Editor を並べる） */
		void DrawFeatureEditors();
		/** 表示（当たり判定などの重ね描き・霧の煙・鏡のガラス・地形と奥の鏡） */
		void DrawDisplay(const InGameStatus& status);
		/** 霧の煙の調整 */
		void DrawMistEffect(const InGameStatus& status);
		/** 地形の見た目と、奥の鏡 */
		void DrawStageView();

		/** 調整する相手がそろっているか（Init の前は何も描かない） */
		bool IsReady() const;

	private:
		/** 調整する相手 */
		InGameEditTargets m_targets;
		/** ウィンドウを出しているか */
		bool m_isOpen = true;
		/** 死んだら自動でリスタートするか */
		bool m_isAutoRestart = true;

		// ---- 機能ごとの調整 ----
		actor::PlayerEditor m_playerEditor;
		camera::CameraEditor m_cameraEditor;
		render::OutlineEditor m_outlineEditor;
		render::MirrorGlassEditor m_mirrorGlassEditor;
		GameplaySEEditor m_seEditor;
		stage::GimmickMarkEditor m_gimmickMarkEditor;
	};
}
