/**
 * @file DarkZone.h
 * @brief 暗がり（矩形の範囲を暗くする。光の近くとプレイヤーのまわりだけ明るく見える）
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
		 *          ・範囲の手前に黒い半透明の板を置いて暗くする（DarknessRenderer が描く）。
		 *            光の線分から lightRadius の中・プレイヤーのまわり playerLightRadius の中・
		 *            見えるようになった「暗がりの足場（HiddenFloor）」の上は、暗さが抜けて見える。
		 *          ・プレイヤーのまわりは、マリオの暗闇ステージのように、いつも丸く見えている。
		 *            isPlayerRevealingFloors が true なら、その丸の中に入った暗がりの足場も見えるようになる。
		 *          ・暗がりの中にある HiddenFloor は、光から lightRadius の中に入ると見えるようになる
		 *            （見た目の明るい範囲と、足場が見える範囲を同じにするため、半径はこのクラスが持つ）。
		 */
		class DarkZone : public RectGimmick
		{
		public:
			/** 暗さ（0=暗くしない 〜 1=真っ黒） */
			float darkness = 0.93f;
			/** 光が照らす半径（光の線分からこの距離までが明るくなり、暗がりの足場が見える） */
			float lightRadius = 120.0f;
			/** プレイヤーのまわりの明るい丸の半径（この中はいつも見える） */
			float playerLightRadius = 260.0f;
			/** プレイヤーのまわりの丸の端のぼかし（半径に対する割合。0 ならくっきり、1 なら真ん中から少しずつ暗くなる） */
			float playerLightSoftness = 0.25f;
			/** プレイヤーのまわりの丸の中に入った暗がりの足場も見えるようにするか（false なら光で照らさないと見えない） */
			bool isPlayerRevealingFloors = true;
			/** 範囲の端をぼかす幅（0 ならくっきり） */
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
			/** プレイヤーのまわりで、暗がりの足場が見える距離（丸の、ぼかしの真ん中まで） */
			float GetPlayerRevealDistance() const { return playerLightRadius * (1.0f - math::Clamp01(playerLightSoftness) * 0.5f); }


		public:
			/** x, y は左下 */
			DarkZone(const char* name, float x, float y, float width, float height);
		};
	}
}
