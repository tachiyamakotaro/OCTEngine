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
		 *          光と平行（鏡がまっすぐ光の向きを向いている）なときは、鏡の端（厚み）に当たって止まる。
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
			/**
			 * @brief 鏡と平行に進む光が、鏡の端（厚み）に当たるか（当たったら光は止まる）
			 * @details 鏡がまっすぐ光の向きを向いているとき、線分どうしの交差では当たらないので、ここで調べる
			 */
			bool IntersectEdge(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const;

			math::Vec2 GetAnchor() const override { return center; }
			/** レールに乗っている鏡はレールごと動かす */
			void SetAnchor(const math::Vec2& anchor) override;
			math::Rect GetBounds() const override;
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			EditorLayer GetEditorLayer() const override { return EditorLayer::Front; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;
			/** 鏡の傾きを中心のまわりに90度ずつ回す（プレイヤーが回す Rotate とは別。エディタ用） */
			void Rotate90(const int quarterTurns) override { angleDeg = math::NormalizeDeg(angleDeg + 90.0f * static_cast<float>(quarterTurns)); }


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
			/**
			 * @brief 鏡の中心が動ける範囲（レールの範囲）
			 */
			struct MoveRange
			{
				/** 左端（中心のX） */
				float minX = 0.0f;
				/** 右端（中心のX） */
				float maxX = 0.0f;
				/** 高さ（鏡は横にしか動かないので、中心の高さのまま） */
				float y = 0.0f;

				/** 長さ */
				float Length() const { return maxX - minX; }
				/** 真ん中のX */
				float CenterX() const { return (minX + maxX) * 0.5f; }
			};
			/**
			 * @brief 動かせる範囲が決まっているか（動かせて、レールが ON で、範囲に長さがある）
			 * @details レールを描くかどうかに使う。Rail が OFF の鏡は、どこまでも動かせるので範囲がない
			 */
			bool HasMoveRange() const { return isMovable && hasRail && railMaxX > railMinX; }
			/** 鏡の中心がレールの範囲の外にあるか（つかんだ瞬間にレールへ飛んでしまう） */
			bool IsOutsideRail() const { return center.x < railMinX || center.x > railMaxX; }
			/** レールの範囲を、今の鏡の位置を真ん中にした初めの長さにする（エディタ用） */
			void FitRailToCenter();
			/** 動かせる範囲（HasMoveRange が true のときに使う） */
			MoveRange GetMoveRange() const
			{
				MoveRange range;
				range.minX = railMinX;
				range.maxX = railMaxX;
				range.y = center.y;
				return range;
			}


		public:
			/** x, y は中心 */
			LightMirror(const char* name, float x, float y, float initialAngleDeg, float initialLength);
		};
	}
}
