/**
 * @file Grass.h
 * @brief 草むら（通路をふさぐ。虫眼鏡で集めた光で燃やすと消えて通れる）
 */
#pragma once
#include "BurnableGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 草むら
		 * @details ・燃え尽きるまでは地形（当たり判定 Terrain）で、光も止める。
		 *          ・燃えている間は、真ん中に向かって縮んでいく（当たり判定・光も縮んだ形）。燃え尽きると消える。
		 *          ・触れている草やロープに燃え広がる。草を積み重ねて壁にすると、下から順に燃え広がっていく。
		 */
		class Grass : public BurnableGimmick
		{
		public:
			const char* GetTypeName() const override { return "Grass"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<Grass>(*this); }

			CollisionKind GetCollision(math::Rect& outRect) const override;
			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			/** 燃えている間は真ん中に向かって縮む */
			math::Rect GetCurrentRect() const override;


		protected:
			unsigned int GetEditorColor() const override;


		public:
			/** x, y は左下 */
			Grass(const char* name, float x, float y, float width, float height);
		};
	}
}
