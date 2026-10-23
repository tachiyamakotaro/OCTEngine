/**
 * @file StageEditor.h
 * @brief ImGui のステージエディタ
 * @details 使い方
 *          ・上：保存・読み込みなどのツールバーと、ギミックの追加ボタン（2行に並べる）
 *          ・左：物の一覧（クリックで選択。Ctrl+クリックで選択に足す・外す）
 *          ・真ん中：ステージの見取り図
 *              左ドラッグ（物の上） … 選択して動かす（Snap ON ならグリッドに吸着）。選択中の物をつかむと、選択中の物を全部まとめて動かす
 *              左ドラッグ（何もないところ） … 範囲選択（四角に少しでもかかった物を選ぶ。Shift / Ctrl を押しながらだと今の選択に足す）
 *              Ctrl+クリック … その物を選択に足す・外す
 *              右/中ドラッグ … 画面の移動、ホイール … 拡大縮小
 *          ・キー（エディタのウィンドウを触っているとき）
 *              Delete … 選択中の物を全部消す　Ctrl+D … 複製　Ctrl+A … 全部選ぶ　Esc … 選択を外す　R / Shift+R … 回転
 *          ・右：詳細パネル（Inspector。Unity の Inspector のように、選択中の物の名前・数値と、複製・削除・回転のボタン）
 *              見取り図との境目をドラッグすると幅を変えられる
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
			/** 追加ボタン（GimmickRegistry.cpp の一覧を2行に分けて並べる） */
			void DrawAddButtons(Stage& stage);
			void DrawObjectList(Stage& stage);
			/** 右の詳細パネル（選択中の物の見出し・編集ボタン・数値） */
			void DrawInspectorPanel(Stage& stage);
			/** 選択中の物の数値（詳細パネルの中身） */
			void DrawProperties(Stage& stage);
			/** 見取り図と詳細パネルの境目（ドラッグで詳細パネルの幅を変える） */
			void DrawInspectorSplitter(float height);
			void DrawCanvas(Stage& stage);
			/** 保存・読み込みの結果を出す */
			void DrawFileMessages();

			/** 選択を全部外す */
			void SelectNone();
			/** 1つだけ選ぶ（今までの選択は外す） */
			void SelectSingle(const Selection& selection);
			/** ギミックを選択に足す・外す（Ctrl+クリック） */
			void ToggleGimmickSelection(int index);
			/** ギミックが選択に入っているか */
			bool IsGimmickSelected(int index) const;
			/** 四角に少しでもかかったギミックを選ぶ（isAdditive なら今の選択に足す） */
			void SelectGimmicksInRect(const Stage& stage, const math::Rect& rect, bool isAdditive);
			/** ギミックを全部選ぶ */
			void SelectAll(const Stage& stage);
			/** 消えた物を指していたら選択から外す（毎フレーム・Undo のあと） */
			void ValidateSelection(const Stage& stage);
			/** 選択中の物をつかんだ（今の位置を覚えて、ドラッグで動かし始める） */
			void BeginObjectDrag(const Stage& stage);
			/** 選択中の物を全部動かす（主の物の基準点が anchor になるように、ほかの物も同じだけ動かす） */
			void MoveSelection(Stage& stage, const math::Vec2& anchor);

			/** 新しいギミックを見取り図の中心に追加して選択する */
			void AddGimmick(Stage& stage, const GimmickRegistryEntry& entry);
			/** 選択中の物を全部複製する（複製した物を選ぶ） */
			void DuplicateSelected(Stage& stage);
			/** 選択中の物を全部消す */
			void DeleteSelected(Stage& stage);
			/** 選択中の物を90度ずつ回す（+1 で反時計回り、-1 で時計回り。いくつか選んでいたら、それぞれをその場で回す） */
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
			/** 選択中の物（いくつか選んでいるときは「主」の物。数値の表示・移動矢印・スナップの基準になる） */
			Selection m_selection;
			/** 選択中のギミック（全ギミックの何番目か）。1つだけ選んでいるときも入る。奥の鏡を選んでいるときは空 */
			std::vector<int> m_selectedGimmicks;
			/** プロパティ欄の入力欄（信号の名前の一覧を毎フレーム入れ直す） */
			GimmickInspector m_inspector;
			/** 見取り図の中心に映すワールド座標 */
			math::Vec2 m_viewCenter;
			/** 見取り図の拡大率（1ワールド単位あたりのピクセル） */
			float m_zoom;
			/** 見取り図の画面上の中心 */
			ImVec2 m_canvasCenter;
			/** 見取り図の画面上の大きさ（Fit View で使う。前のフレームの大きさ） */
			ImVec2 m_canvasSize;
			/** 右の詳細パネルの幅（境目のドラッグで変わる） */
			float m_inspectorWidth;
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
			/** ドラッグ開始時の、選択中のギミックそれぞれの基準点（m_selectedGimmicks と同じ並び） */
			std::vector<math::Vec2> m_dragStartAnchors;
			/** 選択中の物をクリックしただけ（動かさなかった）なら、離したときにその物だけを選び直す */
			bool m_isPendingSingleSelect;

			/** 範囲選択のドラッグ中か */
			bool m_isBoxSelecting;
			/** 範囲選択の始点（ワールド） */
			math::Vec2 m_boxStart;
			/** 範囲選択を今の選択に足すか（始めたときに Shift / Ctrl を押していた） */
			bool m_isBoxAdditive;

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
