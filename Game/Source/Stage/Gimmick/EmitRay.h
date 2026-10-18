/**
 * @file EmitRay.h
 * @brief 光を出すギミック（向いている方向にまっすぐ光を出す）
 */
#pragma once
#include "GimmickBase.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 光を出すギミック（光源）
		 */
		class EmitRay : public GimmickBase
		{
		public:
			/** 光が出る位置 */
			math::Vec2 position;
			/** 光の向き（度。0=右、90=上） */
			float angleDeg = 0.0f;
			/** 光を出しているか */
			bool isOn = true;


		public:
			const char* GetTypeName() const override { return "EmitRay"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<EmitRay>(*this); }

			bool GetEmittedRay(math::Vec2& outOrigin, math::Vec2& outDirection) const override;

			math::Vec2 GetAnchor() const override { return position; }
			void SetAnchor(const math::Vec2& anchor) override { position = anchor; }
			math::Rect GetBounds() const override;
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			EditorLayer GetEditorLayer() const override { return EditorLayer::Front; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 光の向き（長さ1） */
			math::Vec2 GetDirection() const { return math::DirectionFromDeg(angleDeg); }


		public:
			/** x, y は光が出る位置 */
			EmitRay(const char* name, float x, float y, float initialAngleDeg);
		};
	}
}
