/**
 * @file BurnableGimmick.h
 * @brief 燃える物の共通部分（虫眼鏡で集めた光で火がつき、燃え広がり、燃え尽きる）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 燃える物（草・ロープ）の基底クラス
		 * @details 燃え方の流れ（Late の順番で、虫眼鏡が焦点を決めたあとに更新する）
		 *            1. 虫眼鏡の焦点（Magnifier::IsHotSpotIn）が自分の上にある間、温まる（heat が 0 → 1）。
		 *               焦点が外れると、だんだん冷める（coolTime 秒で 1 → 0）
		 *            2. heat が 1 になったら火がつく（isBurning）
		 *            3. 燃えている間、spreadTime 秒たつと、触れている燃える物にも火が移る（燃え広がる）
		 *            4. burnTime 秒で燃え尽きる（isBurnt）。燃え尽きると、自分の名前の信号が ON になる
		 *               （ロープが燃え尽きたら吊るしていた板が落ちる、のように使う）
		 *          ふつうの光（反射鏡の光など）を直接当てても燃えない。虫眼鏡で集めた光だけで燃える。
		 *          見た目（形が縮む・当たり判定）は派生クラスが GetCurrentRect などで決める。
		 */
		class BurnableGimmick : public RectGimmick
		{
		public:
			/** 焦点を当て続けて、火がつくまでの秒数 */
			float igniteTime = 1.0f;
			/** 焦点が外れてから、温まった分が冷めきるまでの秒数 */
			float coolTime = 1.5f;
			/** 火がついてから燃え尽きるまでの秒数 */
			float burnTime = 1.5f;
			/** 火がついてから、触れている燃える物に火が移るまでの秒数（負なら燃え広がらない） */
			float spreadTime = 0.35f;

			/** 【実行時】温まり具合（0〜1。1 で火がつく） */
			float heat = 0.0f;
			/** 【実行時】今フレーム、虫眼鏡の焦点が当たっているか */
			bool isHeated = false;
			/** 【実行時】燃えているか */
			bool isBurning = false;
			/** 【実行時】火がついてからの秒数 */
			float burnElapsed = 0.0f;
			/** 【実行時】燃え尽きたか */
			bool isBurnt = false;
			/** 【実行時】今フレーム火がついたか（エフェクト・SE 用） */
			bool isIgnitedThisFrame = false;
			/** 【実行時】今フレーム燃え尽きたか（エフェクト・SE 用） */
			bool isBurntThisFrame = false;
			/** 【実行時】最後に熱が来た点（焦点の位置、または燃え移ってきた側。煙を出す位置に使う） */
			math::Vec2 heatPoint;


		public:
			/** 虫眼鏡の焦点を見てから動くので Late の順番 */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Late; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;

			/** 燃え尽きたら、自分の名前の信号が ON */
			bool IsSignalSource() const override { return true; }
			bool IsSignalOn() const override { return isBurnt; }

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 燃え具合（0=燃えていない 〜 1=燃え尽きた） */
			float GetBurnProgress() const
			{
				if (isBurnt) { return 1.0f; }
				if (!isBurning || burnTime <= 0.0f) { return 0.0f; }
				return math::Clamp01(burnElapsed / burnTime);
			}
			/** 火をつける（燃えている・燃え尽きた物は何もしない） */
			void Ignite();
			/** 今の形（燃えている間に小さくなる物は override する。燃え尽きたあとは使わない） */
			virtual math::Rect GetCurrentRect() const { return rect; }


		protected:
			/** 焦点がこの距離まで外にあっても温まる（細い物は少し大きめにする） */
			virtual float GetHotSpotMargin() const { return 4.0f; }
			/** 見取り図の色（燃えていないとき。ImGui の IM_COL32 の値） */
			virtual unsigned int GetEditorColor() const = 0;


		protected:
			/** x, y は左下 */
			BurnableGimmick(const char* name, float x, float y, float width, float height);
		};
	}
}
