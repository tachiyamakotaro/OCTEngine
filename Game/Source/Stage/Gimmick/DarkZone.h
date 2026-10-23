/**
 * @file DarkZone.h
 * @brief 暗がり（ステージ全体か、矩形の範囲を暗くする。光の近くとプレイヤーのまわりだけ明るく見える）
 */
#pragma once
#include "RectGimmick.h"
#include <vector>


namespace app
{
	namespace stage
	{
		/**
		 * @brief 暗がり
		 * @details ・見た目だけのギミック。当たり判定はなく、光も止めない。
		 *          ・isWholeStage が true なら、ステージ全体を暗くする（範囲 rect は、エディタで選ぶための枠として残すだけ）。
		 *            false なら範囲 rect の中だけを暗くする（範囲の端は edgeFeather でぼかす）。
		 *          ・手前に黒い半透明の板を置いて暗くする（DarknessRenderer が描く）。
		 *            光の線分のまわり（lightRadius）・プレイヤーのまわり（playerLightRadius）・見えた暗がりの足場（HiddenFloor）の上は、
		 *            暗さが抜けて見える。明るい所と暗い所の境目は、ぼかしの幅（lightSoftWidth / playerLightSoftWidth）で
		 *            少しずつ暗くなるので、くっきり分かれない。
		 *          ・プレイヤーのまわりは、マリオの暗闇ステージのように、いつも丸く見えている。
		 *            isPlayerRevealingFloors が true なら、その丸の中に入った暗がりの足場も見えるようになる。
		 *          ・slantLeft / slantRight で、左右の端を斜めにできる（ステージ全体のときは使わない）。
		 *            rect の下の辺はそのままで、上の辺の端だけを横にずらす（平行四辺形・台形になる）。
		 *            明るい部屋から斜めに差しこむ光が、暗い部屋の奥までは届いていない見た目にするのに使う。
		 *            例：左の明るい部屋から光が差しこむなら slantLeft をマイナスにする（上ほど左＝床に近いほど光が奥まで届く）。
		 *          ・bendLeft / bendRight で、左右の端の途中に「折れ目」を作れる（端が、折れ目でつながった2本のまっすぐな線になる）。
		 *            折れ目の高さは bendLeftHeight / bendRightHeight（下の辺 0 〜 上の辺 1。初期値 0.5 = 真ん中）。
		 *            bend は、折れ目の高さで、まっすぐな端からどれだけ右へずらすか（マイナスで左）。上と下の角は動かない。
		 *            例：左の端の折れ目を左へ（bendLeft をマイナス）→ 暗がりが左に広がる（光が入ってくる所がせまくなる）。
		 *            入口の縁（段の角など）に折れ目を合わせると、縁に沿った影にできる。
		 *          ・見取り図で選ぶと、上の2つの角と、左右の折れ目に取っ手が出る。
		 *            角は横に、折れ目は上下左右にドラッグできる（slant / bend / 折れ目の高さが変わる）。
		 *          ・暗がりの足場の見え具合は、足場の場所の明るさと同じ計算（Glow）で決める（見た目の明るさと、足場が見えるかがずれない）。
		 *            光が離れたら、その場ですぐ暗くなる（明るいまま残らない）。
		 */
		class DarkZone : public RectGimmick
		{
		public:
			/** ステージ全体を暗くするか（false なら範囲 rect の中だけ） */
			bool isWholeStage = true;
			/** 暗さ（0=暗くしない 〜 1=真っ黒） */
			float darkness = 0.93f;
			/** 光が照らす半径（光の線分からこの距離のあたりが、明るい所と暗い所の境目の真ん中） */
			float lightRadius = 90.0f;
			/** 光のまわりの境目のぼかしの幅（境目の真ん中から内側と外側に半分ずつ、少しずつ暗くなる） */
			float lightSoftWidth = 100.0f;
			/** プレイヤーのまわりの明るい丸の半径（境目の真ん中） */
			float playerLightRadius = 200.0f;
			/** プレイヤーのまわりの丸の境目のぼかしの幅 */
			float playerLightSoftWidth = 100.0f;
			/** プレイヤーのまわりの丸の中に入った暗がりの足場も見えるようにするか（false なら光で照らさないと見えない） */
			bool isPlayerRevealingFloors = true;
			/** 範囲の端をぼかす幅（isWholeStage が false のときだけ使う。0 ならくっきり） */
			float edgeFeather = 40.0f;
			/**
			 * 左の端の傾き：上の辺で、左の端を下の辺の左の端からどれだけ右へずらすか（マイナスで左。0 ならまっすぐ）
			 * （isWholeStage が false のときだけ使う）
			 */
			float slantLeft = 0.0f;
			/** 右の端の傾き：上の辺で、右の端を下の辺の右の端からどれだけ右へずらすか（マイナスで左。0 ならまっすぐ） */
			float slantRight = 0.0f;
			/**
			 * 左の端の折れ目：折れ目の高さで、まっすぐな端からどれだけ右へずらすか（マイナスで左。0 ならまっすぐ）
			 * （isWholeStage が false のときだけ使う）
			 */
			float bendLeft = 0.0f;
			/** 右の端の折れ目：折れ目の高さで、まっすぐな端からどれだけ右へずらすか（マイナスで左。0 ならまっすぐ） */
			float bendRight = 0.0f;
			/** 左の端の折れ目の高さ（下の辺 0 〜 上の辺 1） */
			float bendLeftHeight = 0.5f;
			/** 右の端の折れ目の高さ（下の辺 0 〜 上の辺 1） */
			float bendRightHeight = 0.5f;


		public:
			const char* GetTypeName() const override { return "DarkZone"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<DarkZone>(*this); }

			/** 一番後ろに描く（ほかのギミックが見えるように） */
			EditorLayer GetEditorLayer() const override { return EditorLayer::Back; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;
			/** 枠の中をクリックしても選ばない（中に置いた足場などを選びやすくする）。枠の線の近くだけ */
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			/** 取っ手：0 = 左の端の折れ目, 1 = 右の端の折れ目, 2 = 左上の角, 3 = 右上の角（ステージ全体のときはなし） */
			int GetEditHandleCount() const override { return isWholeStage ? 0 : 4; }
			math::Vec2 GetEditHandle(int index) const override;
			void SetEditHandle(int index, const math::Vec2& position) override;


		public:
			/**
			 * @brief 明るさ（1 = 明るい 〜 0 = 暗い）：境目の真ん中 radius から内側と外側に softWidth の半分ずつ、なめらかに変わる
			 * @details シェーダー（darkness.fx の DarkGlow）と同じ計算
			 */
			static float Glow(float distance, float radius, float softWidth);
			/** 光の線分から distance 離れた所の明るさ */
			float GetLightGlow(float distance) const { return Glow(distance, lightRadius, lightSoftWidth); }
			/** プレイヤーから distance 離れた所の明るさ */
			float GetPlayerGlow(float distance) const { return Glow(distance, playerLightRadius, playerLightSoftWidth); }
			/** 光が届く一番遠い距離（ここより遠いと、まったく明るくならない） */
			float GetLightReach() const { return lightRadius + math::Max(lightSoftWidth, 0.0f) * 0.5f; }
			/** 点が暗がりの中か（ステージ全体なら、いつも中。斜めの端も考える） */
			bool Contains(const math::Vec2& point) const;
			/** 高さ y での、左の端の X（斜めの端。y が範囲の外なら、下の辺か上の辺の値） */
			float GetLeftX(float y) const;
			/** 高さ y での、右の端の X */
			float GetRightX(float y) const;
			/** 折れ目の高さ（0〜1）から、折れ目のワールドの Y を求める */
			float GetBendY(float bendHeight) const;
			/** 4つの角（左下・右下・右上・左上の順。斜めの端も考える） */
			void GetCorners(math::Vec2 outCorners[4]) const;
			/** 斜めの端も入れた、暗がり全体を囲む矩形（板の大きさや、光の線分を渡すかを決めるのに使う） */
			math::Rect GetDarkBounds() const;
			/** 端が斜め（または曲がっている）か */
			bool IsSlanted() const { return slantLeft != 0.0f || slantRight != 0.0f || bendLeft != 0.0f || bendRight != 0.0f; }
			/**
			 * @brief 端の形を、線をつないだ形（左下から反時計回り）で out に入れる（見取り図に描く・クリックを調べるのに使う）
			 */
			void GetOutline(std::vector<math::Vec2>& out) const;


		public:
			/** x, y は左下 */
			DarkZone(const char* name, float x, float y, float width, float height);
		};
	}
}
