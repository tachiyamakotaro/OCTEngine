/**
 * @file LightPlatform.h
 * @brief 光の足場（光が当たっている間だけ乗れる）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 光の足場
		 * @details 光は止めずに通り抜ける。反射鏡を動かすときはすり抜ける（地形ではない）。
		 */
		class LightPlatform : public RectGimmick
		{
		public:
			/** 光が外れてから消えるまでの猶予（ちらつき防止） */
			float lingerTime = 0.15f;

			/** 【実行時】今フレーム光が当たっているか */
			bool isLit = false;
			/** 【実行時】足場として残っている時間 */
			float solidTimer = 0.0f;


		public:
			const char* GetTypeName() const override { return "LightPlatform"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<LightPlatform>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 足場として乗れるか */
			bool IsSolid() const { return solidTimer > 0.0f; }


		public:
			/** x, y は左下 */
			LightPlatform(const char* name, float x, float y, float width, float height);
		};
	}
}
