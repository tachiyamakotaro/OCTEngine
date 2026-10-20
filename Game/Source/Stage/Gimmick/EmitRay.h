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
			/** 太い光を出すか（false ならふつうの細い光） */
			bool isWideBeam = false;
			/** 太い光の太さ（偽物の壁を Band にすると、この太さで壁が消える。プレイヤーが通るなら体の高さより少し大きく） */
			float wideBeamWidth = 140.0f;


		public:
			const char* GetTypeName() const override { return "EmitRay"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<EmitRay>(*this); }

			bool GetEmittedRay(math::Vec2& outOrigin, math::Vec2& outDirection) const override;
			float GetEmittedBeamWidth() const override { return isWideBeam ? wideBeamWidth : 0.0f; }

			math::Vec2 GetAnchor() const override { return position; }
			void SetAnchor(const math::Vec2& anchor) override { position = anchor; }
			math::Rect GetBounds() const override;
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			EditorLayer GetEditorLayer() const override { return EditorLayer::Front; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;
			/** 光の向きを90度ずつ回す */
			void Rotate90(const int quarterTurns) override { angleDeg = math::NormalizeDeg(angleDeg + 90.0f * static_cast<float>(quarterTurns)); }


		public:
			/** 光の向き（長さ1） */
			math::Vec2 GetDirection() const { return math::DirectionFromDeg(angleDeg); }


		public:
			/** x, y は光が出る位置 */
			EmitRay(const char* name, float x, float y, float initialAngleDeg);
		};
	}
}
