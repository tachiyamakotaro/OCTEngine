/**
 * @file StageEditor.h
 * @brief ImGui のステージエディタ
 * @details 使い方
 *          ・左：物の一覧（クリックで選択）と、選択中の物の数値
 *          ・右：ステージの見取り図
 *              左ドラッグ … 選択して動かす（Snap ON ならグリッドに吸着）
 *              右/中ドラッグ … 画面の移動、ホイール … 拡大縮小
 *          ・「Export C++」… 今の配置を C++ コードにしてクリップボードにコピーする。
 *              ステージ定義（Stage01.cpp など）の関数に貼り戻せば、調整結果がコードに残る。
 *
 *          ImGui に日本語フォントを読み込んでいないため、画面上の表示は英語にしている。
 */
#pragma once
#include "imgui.h"
#include "Source/Core/Math/Vec2.h"
#include "Source/Stage/Stage.h"
#include <string>
#include <utility>
#include <vector>


namespace app
{
	namespace actor
	{
		class CharacterBase;
	}


	namespace stage
	{
		class Stage;


		/**
		 * @brief ステージエディタ
		 */
		class StageEditor
		{
		public:
			/** ウィンドウを表示するか */
			bool isOpen = true;


		public:
			/**
			 * @brief エディタのウィンドウを描く（ImGui::NewFrame と ImGui::Render の間で毎フレーム呼ぶ）
			 */
			void Draw(Stage& stage);
			/**
			 * @brief 見取り図に表示するキャラクターを登録する（プレイヤー・鏡の中の自分）
			 */
			void AddCharacter(const char* label, const actor::CharacterBase* character);
			/**
			 * @brief 今のステージを C++ のステージ定義コードにする
			 * @param functionName 関数名
			 */
			std::string GenerateCppCode(const Stage& stage, const char* functionName) const;


		public:
			StageEditor();


		private:
			/** 選択できる物の種類 */
			enum class ObjectType
			{
				None,
				Block,
				LightSource,
				LightMirror,
				LightReceiver,
				Door,
				LightPlatform,
				BackMirror,
				FloorSwitch,
				MistEmitter,
				MirrorPlatform,
				Checkpoint,
				WarpZone,
				Indicator,
				Goal,
			};

			enum class GizmoAxis
			{
				None,
				X,
				Y
			};

			/** 選択中の物 */
			struct Selection
			{
				ObjectType type = ObjectType::None;
				int index = -1;
			};

			/** Undo/Redo 用に保存するステージの中身 */
			struct Snapshot
			{
				std::string name;
				BackMirror backMirror;
				std::vector<StageBlock> blocks;
				std::vector<LightSource> lightSources;
				std::vector<LightMirror> lightMirrors;
				std::vector<LightReceiver> lightReceivers;
				std::vector<Door> doors;
				std::vector<LightPlatform> lightPlatforms;
				std::vector<FloorSwitch> floorSwitches;
				std::vector<MistEmitter> mistEmitters;
				std::vector<MirrorPlatform> mirrorPlatforms;
				std::vector<Checkpoint> checkpoints;
				std::vector<WarpZone> warpZones;
				std::vector<Indicator> indicators;
				std::vector<Goal> goals;
			};


		private:
			void DrawToolbar(Stage& stage);
			void DrawObjectList(Stage& stage);
			void DrawProperties(Stage& stage);
			void DrawCanvas(Stage& stage);
			void DrawExportWindow();

			/** 新しい物を見取り図の中心に追加して選択する */
			void AddObject(Stage& stage, ObjectType type);
			/** 選択中の物を複製する */
			void DuplicateSelected(Stage& stage);
			/** 選択中の物を消す */
			void DeleteSelected(Stage& stage);
			/** 選択が有効か（消された物を指していないか） */
			bool IsSelectionValid(const Stage& stage) const;
			/** 指定した位置にある物を探す */
			Selection HitTest(const Stage& stage, const math::Vec2& worldPos) const;
			/** 選択中の物の基準点（ドラッグ用） */
			math::Vec2 GetAnchor(const Stage& stage, const Selection& selection) const;
			/** 選択中の物の基準点を動かす */
			void SetAnchor(Stage& stage, const Selection& selection, const math::Vec2& anchor);
			/** 重ならない名前を作る（"Block3" など） */
			std::string MakeUniqueName(const Stage& stage, const char* prefix) const;
			/** 信号（受光器・スイッチ）の名前を変えたとき、扉と霧のつながりも付け替える */
			void RenameSignal(Stage& stage, const std::string& oldName, const std::string& newName);
			/** 信号（受光器・スイッチ）を消したとき、扉と霧のつながりから外す */
			void RemoveSignal(Stage& stage, const std::string& name);
			/** 信号（受光器・スイッチ）を選ぶコンボボックス（空も選べる） */
			void SignalCombo(const Stage& stage, const char* label, std::string& signal);
			/** グリッドに吸着 */
			float Snap(float value) const;

			/** ワールド座標 → 見取り図の画面座標 */
			ImVec2 WorldToScreen(const math::Vec2& world) const;
			/** 見取り図の画面座標 → ワールド座標 */
			math::Vec2 ScreenToWorld(const ImVec2& screen) const;

			GizmoAxis HitTestGizmo(const Stage& stage, const ImVec2& mousePos) const;
			void DrawGizmo(ImDrawList* drawList, const Stage& stage, GizmoAxis hovered) const;


			/** 今のステージを保存する／保存した状態に戻す */
			Snapshot CaptureSnapshot(const Stage& stage) const;
			void RestoreSnapshot(Stage& stage, const Snapshot& snapshot);
			/** 変更があったら履歴に積む（毎フレーム） */
			void UpdateHistory(const Stage& stage);
			void Undo(Stage& stage);
			void Redo(Stage& stage);
			/** Ctrl+Z / Ctrl+Y */
			void HandleShortcuts(Stage& stage);

		private:
			/** 選択中の物 */
			Selection m_selection;
			/** 見取り図の中心に映すワールド座標 */
			math::Vec2 m_viewCenter;
			/** 見取り図の拡大率（1ワールド単位あたりのピクセル） */
			float m_zoom;
			/** 見取り図の画面上の中心 */
			ImVec2 m_canvasCenter;
			/** グリッドに吸着するか */
			bool m_isSnap;
			/** グリッドの大きさ */
			float m_gridSize;
			/** 光を表示するか */
			bool m_isShowLight;
			/** 奥の鏡の枠を表示するか */
			bool m_isShowBackMirror;

			/** ドラッグで動かしているか */
			bool m_isDraggingObject;
			/** ドラッグ開始時の基準点 */
			math::Vec2 m_dragStartAnchor;
			/** ドラッグ開始からのマウスの移動量（ワールド） */
			math::Vec2 m_dragAccumulated;
			GizmoAxis m_dragAxis;

			/** 表示するキャラクター */
			std::vector<std::pair<std::string, const actor::CharacterBase*>> m_characters;

			/** 出力したコード */
			std::string m_exportText;
			/** 出力ウィンドウを表示するか */
			bool m_isShowExport;
			/** 出力する関数名 */
			char m_functionName[64];

			/** 元に戻す／やり直す履歴 */
			std::vector<Snapshot> m_undoStack;
			std::vector<Snapshot> m_redoStack;
			/** 最後に落ち着いた（操作が終わった）状態と、その比較用コード */
			Snapshot m_stableSnapshot;
			std::string m_stableCode;
			/** 最初の状態を保存したか */
			bool m_isHistoryReady = false;
		};
	}
}
