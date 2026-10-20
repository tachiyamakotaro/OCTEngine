/**
 * @file ShadeBoard.h
 * @brief 日よけ板（光をさえぎって日陰を作る。プレイヤーがつかんで左右に動かせる）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 日よけ板
		 * @details ・光を止める（Stop）。板の後ろ（光と反対側）が日陰になり、そこにある氷は溶けない。
		 *          ・当たり判定はない（反射鏡と同じく、プレイヤーは通り抜ける）。
		 *          ・プレイヤーが近くでつかむ（RB1）を押したまま歩くと、引きずって左右に動かせる（ShadeInteractor）。
		 *          ・頭の上の高さに横長に置けば「日傘」、床に縦長に立てれば「ついたて」になる。
		 *            つかめる高さは、板の下の面が足元から ShadeInteractor::interactRangeY まで。
		 */
		class ShadeBoard : public RectGimmick
		{
		public:
			/** プレイヤーが動かせるか */
			bool isMovable = true;
			/** 動かせる範囲を制限するか */
			bool hasRail = false;
			/** 動かせる範囲の左端（板の中心のX） */
			float railMinX = 0.0f;
			/** 動かせる範囲の右端（板の中心のX） */
			float railMaxX = 0.0f;


		public:
			const char* GetTypeName() const override { return "ShadeBoard"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<ShadeBoard>(*this); }

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			/** レールがある板はレールごと動かす */
			void SetAnchor(const math::Vec2& anchor) override;
			EditorLayer GetEditorLayer() const override { return EditorLayer::Front; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 板の中心のX */
			float GetCenterX() const { return (rect.min.x + rect.max.x) * 0.5f; }
			/** 中心のXを動かす（大きさはそのまま） */
			void SetCenterX(const float x)
			{
				const float dx = x - GetCenterX();
				rect.min.x += dx;
				rect.max.x += dx;
			}
			/** レールの範囲に収めた中心のX */
			float ClampToRail(const float x) const
			{
				return hasRail ? math::Clamp(x, railMinX, railMaxX) : x;
			}


		public:
			/** x, y は左下 */
			ShadeBoard(const char* name, float x, float y, float width, float height);
		};
	}
}
