/**
 * @file StageEditor.h
 * @brief ImGui のステージエディタ
 * @details 使い方
 *          ・左：物の一覧（クリックで選択）と、選択中の物の数値
 *          ・右：ステージの見取り図
 *              左ドラッグ … 選択して動かす（Snap ON ならグリッドに吸着）
 *              右/中ドラッグ … 画面の移動、ホイール … 拡大縮小
 *          ・「Save JSON」… 今の配置を JSON ファイル（StageCatalog.h の地形のファイル・ギミックのファイル）に保存する。
 *          ・「Load JSON」… JSON ファイルから読み直す（保存していない変更は消えるが、Undo で戻せる）。
 *          ・何も選んでいないときは、ステージの設定（名前・プレイヤーの開始位置）を変えられる。
 *
 *          ImGui に日本語フォントを読み込んでいないため、画面上の表示は英語にしている。
 */
#pragma once
#include "imgui.h"
#include "Source/Core/Math/Vec2.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/StageCatalog.h"
#include "Source/Stage/Gimmick/GimmickEditTools.h"
#include "Source/Stage/Gimmick/GimmickRegistry.h"
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
			 * @brief 保存・読み込みに使うファイルを決める（DebugScene が今のステージの情報を渡す）
			 */
			void SetStageInfo(const StageInfo* info) { m_stageInfo = info; }


		public:
			StageEditor();


		private:
			/** 選択している物の種類 */
			enum class SelectionType
			{
				None,
				BackMirror,	//!< 奥の鏡（ステージに1つ）
				Gimmick,	//!< ギミック（index は全ギミックの何番目か）
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
				SelectionType type = SelectionType::None;
				int index = -1;
			};

			/** Undo/Redo 用に保存するステージの中身（GimmickManager はコピーすると中身も全部コピーされる） */
			struct Snapshot
			{
				std::string name;
				math::Vec2 playerStart;
				BackMirror backMirror;
				GimmickManager gimmicks;
			};


		private:
			void DrawToolbar(Stage& stage);
			void DrawObjectList(Stage& stage);
			void DrawProperties(Stage& stage);
			void DrawCanvas(Stage& stage);
			/** 保存・読み込みの結果を出す */
			void DrawFileMessages();

			/** 新しいギミックを見取り図の中心に追加して選択する */
			void AddGimmick(Stage& stage, const GimmickRegistryEntry& entry);
			/** 選択中の物を複製する */
			void DuplicateSelected(Stage& stage);
			/** 選択中の物を消す */
			void DeleteSelected(Stage& stage);
			/** 選択中の物を90度ずつ回す（+1 で反時計回り、-1 で時計回り） */
			void RotateSelected(Stage& stage, int quarterTurns);
			/** 選択が有効か（消された物を指していないか） */
			bool IsSelectionValid(const Stage& stage) const;
			/** 指定した位置にある物を探す */
			Selection HitTest(const Stage& stage, const math::Vec2& worldPos) const;
			/** 選択中の物の基準点（ドラッグ用） */
			math::Vec2 GetAnchor(const Stage& stage, const Selection& selection) const;
			/** 選択中の物の基準点を動かす */
			void SetAnchor(Stage& stage, const Selection& selection, const math::Vec2& anchor);
			/** 選択中のギミック（ギミックを選んでいなければ nullptr） */
			GimmickBase* GetSelectedGimmick(Stage& stage) const;
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
			/** プロパティ欄の入力欄（信号の名前の一覧を毎フレーム入れ直す） */
			GimmickInspector m_inspector;
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

			/** 保存・読み込みに使うファイル（DebugScene が渡す） */
			const StageInfo* m_stageInfo = nullptr;
			/** 保存・読み込みの結果のメッセージ */
			std::vector<std::string> m_fileMessages;
			/** 保存・読み込みに成功したか */
			bool m_isFileSucceeded = true;

			/** 元に戻す／やり直す履歴 */
			std::vector<Snapshot> m_undoStack;
			std::vector<Snapshot> m_redoStack;
			/** 最後に落ち着いた（操作が終わった）状態と、その比較用の JSON */
			Snapshot m_stableSnapshot;
			std::string m_stableCode;
			/** 最初の状態を保存したか */
			bool m_isHistoryReady = false;
		};
	}
}
