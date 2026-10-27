/**
 * @file MirrorPromptUI.h
 * @brief 反射鏡の近くで出すボタンのインタラクト表示（B で回転 / RB で移動）
 * @details 見た目
 *          ・鏡の上に、暗い半透明の板に乗せた「ボタン＋何が起きるかのアイコン」を出す
 *              回転できる鏡 … [B ↻]（プレイヤーが左を向いているときは ↺。押すと回る向きと同じ）
 *              動かせる鏡   … [RB ⟷]
 *              両方できる鏡 … [B ↻] [RB ⟷] を横に並べる
 *          ・板の下の小さな三角が、操作できる鏡を指す
 *          ・近づくと少し大きい状態から縮んで出てくる（ポップ）。離れると薄くなって消える
 *          ・B を押した瞬間：B が跳ねて、回転のアイコンが1回転する
 *          ・RB で引きずっている間：RB が黄色くなって少し縮み（押している見た目）、左右の矢印が揺れる
 *
 *          どの鏡を操作できるかは MirrorInteractor（GetFocusedMirrorIndex）を見るだけで、何も変えない。
 *          座標は画面の真ん中が原点、上が +Y（画面は 1920x1080）。鏡のワールド座標をカメラで画面の位置に変えて置く。
 *          画像は Assets/sprite/prompt/。
 */
#pragma once
#include "Source/UI/UISprite.h"
#include <array>


namespace app
{
	namespace actor
	{
		class CharacterBase;
	}


	namespace stage
	{
		class Stage;
		class MirrorInteractor;
	}


	namespace ui
	{
		/**
		 * @brief 反射鏡のインタラクト表示
		 */
		class MirrorPromptUI
		{
		public:
			MirrorPromptUI();
			~MirrorPromptUI();
			MirrorPromptUI(const MirrorPromptUI&) = delete;
			MirrorPromptUI& operator=(const MirrorPromptUI&) = delete;

			/** 画像を読み込む（シーンの Start で1回） */
			void Init();
			/**
			 * @brief 見た目を更新する（毎フレーム。鏡の操作とカメラの Update のあとに呼ぶ）
			 * @param stage ステージ（操作対象の鏡を取り出す）
			 * @param interactor 反射鏡の操作（どの鏡が操作できるか・回したか・引きずっているか）
			 * @param character 操作しているキャラクター（向きで回転のアイコンを変える）
			 * @param canGrab RB で鏡をつかめるか（日よけ板が近くて RB が板のほうに使われるときは false）
			 * @param isAllowed 出してよいか（ゴールしたあと・時間切れの間などは false）
			 * @param deltaTime 演出に使う時間
			 */
			void Update(const stage::Stage& stage, const stage::MirrorInteractor& interactor,
				const actor::CharacterBase& character, bool canGrab, bool isAllowed, float deltaTime);
			/** 描く（シーンの Render から。2D の画像は 3D のあとに描かれる） */
			void Render(RenderContext& rc);

			/** 表示するか（デバッグで隠したいとき用） */
			void SetVisible(bool isVisible) { m_isVisible = isVisible; }
			bool IsVisible() const { return m_isVisible; }

			/** ImGui の調整項目（表示の ON / OFF・鏡からの高さ）。見出しは呼ぶ側で作る */
			void DrawSettingsUI();


		private:
			/** 並べる項目（1つの板） */
			enum class EntryKind
			{
				Rotate,		// B で回転
				Move,		// RB で移動
			};
			/** 並べられる項目の数 */
			static constexpr int MAX_ENTRY_COUNT = 2;

			/**
			 * @brief 鏡の上の点（ワールド）を画面の位置に変える
			 * @return カメラの後ろなら false
			 */
			bool WorldToScreen(float worldX, float worldY, float& outX, float& outY) const;
			/** 全部の画像を置く（並べる項目・位置・演出をまとめて反映する） */
			void PlaceAll();


		private:
			/** 表示するか（デバッグ用） */
			bool m_isVisible;
			/** 鏡のいちばん上から、表示の下の端（三角の先）までの高さ（ワールドの長さ） */
			float m_anchorOffsetY;

			// ---- 今の状態 ----
			/** 前のフレームに操作できた鏡（全ギミックの何番目か。なければ -1） */
			int m_prevIndex;
			/** 並べる項目 */
			std::array<EntryKind, MAX_ENTRY_COUNT> m_entries;
			/** 並べる項目の数（0 なら出さない。消えていく間は前の項目を残す） */
			int m_entryCount;
			/** 三角の先の画面の位置（消えていく間は最後の位置に残す） */
			float m_anchorX;
			float m_anchorY;
			/** 回転のアイコンを時計回りにするか（MirrorInteractor::ROTATE_STEPS で決まる） */
			bool m_isClockwise;
			/** 引きずっているか */
			bool m_isDragging;

			// ---- 演出 ----
			/** 見えている割合（0〜1） */
			float m_alpha;
			/** 出てきてからの時間（ポップの演出） */
			float m_popTimer;
			/** B を押したときの演出の残り時間 */
			float m_rotatePulseTimer;
			/** ふわふわ上下するための時間 */
			float m_elapsed;

			// ---- 画像 ----
			/** 背景の板（項目ごとに1枚） */
			std::array<UISprite, MAX_ENTRY_COUNT> m_plates;
			/** 鏡を指す三角 */
			UISprite m_pointer;
			/** B ボタン */
			UISprite m_buttonB;
			/** RB ボタン */
			UISprite m_buttonRB;
			/** 回転のアイコン（時計回り・反時計回り） */
			UISprite m_iconRotateCW;
			UISprite m_iconRotateCCW;
			/** 移動のアイコン（左右の矢印） */
			UISprite m_iconMove;
		};
	}
}
