/**
 * @file LightMirror.h
 * @brief 光を反射する鏡（線分）
 */
#pragma once
#include "GimmickBase.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 光を反射する鏡（線分）
		 * @details 表側（法線の向き）だけが光を反射する。裏側に当たった光は止まる。
		 *          isDoubleSided を true にすると両面で反射する。
		 *          プレイヤーが回したり動かしたりする（MirrorInteractor）。
		 */
		class LightMirror : public GimmickBase
		{
		public:
			/** 鏡の中心 */
			math::Vec2 center;
			/** 鏡面の傾き（度。0=水平で表は上向き、90=垂直で表は左向き） */
			float angleDeg = 45.0f;
			/** 鏡の長さ */
			float length = 80.0f;
			/** 両面で反射するか */
			bool isDoubleSided = false;
			/** プレイヤーが回転できるか */
			bool isRotatable = true;
			/** 1回の回転で回る角度（度） */
			float rotationStepDeg = 45.0f;
			/** プレイヤーが動かせるか */
			bool isMovable = true;
			/** 動かせる範囲を制限するか */
			bool hasRail = false;
			/** 動かせる範囲の左端（中心のX） */
			float railMinX = 0.0f;
			/** 動かせる範囲の右端（中心のX） */
			float railMaxX = 0.0f;


		public:
			const char* GetTypeName() const override { return "LightMirror"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<LightMirror>(*this); }

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			math::Vec2 GetAnchor() const override { return center; }
			/** レールに乗っている鏡はレールごと動かす */
			void SetAnchor(const math::Vec2& anchor) override;
			math::Rect GetBounds() const override;
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			EditorLayer GetEditorLayer() const override { return EditorLayer::Front; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 鏡面に沿った向き */
			math::Vec2 GetDirection() const { return math::DirectionFromDeg(angleDeg); }
			/** 表側の法線 */
			math::Vec2 GetNormal() const
			{
				const math::Vec2 dir = GetDirection();
				return { -dir.y, dir.x };
			}
			/** 端点（始点） */
			math::Vec2 GetStart() const { return center - GetDirection() * (length * 0.5f); }
			/** 端点（終点） */
			math::Vec2 GetEnd() const { return center + GetDirection() * (length * 0.5f); }
			/**
			 * @brief 回転させる
			 * @param steps 回す段数（正で反時計回り）
			 */
			void Rotate(const int steps)
			{
				angleDeg = math::NormalizeDeg(angleDeg + rotationStepDeg * static_cast<float>(steps));
			}
			/** レールの範囲に収めた中心X */
			float ClampToRail(const float x) const
			{
				return hasRail ? math::Clamp(x, railMinX, railMaxX) : x;
			}


		public:
			/** x, y は中心 */
			LightMirror(const char* name, float x, float y, float initialAngleDeg, float initialLength);
		};
	}
}
