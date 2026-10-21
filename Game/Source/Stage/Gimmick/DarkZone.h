/**
 * @file DarkZone.h
 * @brief 暗がり（ステージ全体か、矩形の範囲を暗くする。光の近くとプレイヤーのまわりだけ明るく見える）
 */
#pragma once
#include "RectGimmick.h"


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
			/** 点が暗がりの中か（ステージ全体なら、いつも中） */
			bool Contains(const math::Vec2& point) const;


		public:
			/** x, y は左下 */
			DarkZone(const char* name, float x, float y, float width, float height);
		};
	}
}
