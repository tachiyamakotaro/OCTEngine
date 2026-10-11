/**
 * @file LightTracer.cpp
 * @brief 光の経路を計算する（レイを飛ばして鏡で反射させる）
 */
#include "stdafx.h"
#include "LightTracer.h"
#include <cfloat>
#include "Stage.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/** 反射した直後に同じ面へ当たり直さないための、わずかな距離 */
			constexpr float RAY_EPSILON = 0.01f;
			/** 平行とみなす値 */
			constexpr float PARALLEL_EPSILON = 1e-6f;


			/** 当たった物の種類 */
			enum class HitKind
			{
				None,
				Block,		//!< ブロック・閉じた扉（光が止まる）
				Mirror,		//!< 反射鏡
				Receiver,	//!< 受光器
			};
		}


		bool RayIntersectRect(const math::Vec2& origin, const math::Vec2& direction, const math::Rect& rect, float& outT)
		{
			// スラブ法：X と Y それぞれで「入る距離」「出る距離」を求め、重なりがあれば当たり
			float tEnter = -FLT_MAX;
			float tExit = FLT_MAX;

			const float o[2] = { origin.x, origin.y };
			const float d[2] = { direction.x, direction.y };
			const float mn[2] = { rect.min.x, rect.min.y };
			const float mx[2] = { rect.max.x, rect.max.y };

			for (int axis = 0; axis < 2; ++axis)
			{
				if (math::Abs(d[axis]) < PARALLEL_EPSILON)
				{
					// この軸と平行：始点がスラブの外なら絶対に当たらない
					if (o[axis] <= mn[axis] || o[axis] >= mx[axis])
					{
						return false;
					}
					continue;
				}
				float t0 = (mn[axis] - o[axis]) / d[axis];
				float t1 = (mx[axis] - o[axis]) / d[axis];
				if (t0 > t1) { const float tmp = t0; t0 = t1; t1 = tmp; }
				tEnter = math::Max(tEnter, t0);
				tExit = math::Min(tExit, t1);
				if (tEnter > tExit)
				{
					return false;
				}
			}

			if (tExit < 0.0f)
			{
				// 矩形は後ろにある
				return false;
			}
			outT = tEnter > 0.0f ? tEnter : 0.0f;
			return true;
		}


		bool RayIntersectSegment(const math::Vec2& origin, const math::Vec2& direction, const math::Vec2& a, const math::Vec2& b, float& outT)
		{
			const math::Vec2 edge = b - a;
			const float denom = math::Cross(direction, edge);
			if (math::Abs(denom) < PARALLEL_EPSILON)
			{
				return false;
			}
			const math::Vec2 toA = a - origin;
			const float t = math::Cross(toA, edge) / denom;		// レイ上の距離
			const float s = math::Cross(toA, direction) / denom;	// 線分上の位置（0〜1）
			if (t <= RAY_EPSILON || s < 0.0f || s > 1.0f)
			{
				return false;
			}
			outT = t;
			return true;
		}


		void LightTracer::Trace(const Stage& stage, LightTraceResult& out) const
		{
			out.Clear();
			out.litReceivers.assign(stage.GetLightReceivers().size(), false);
			out.litPlatforms.assign(stage.GetLightPlatforms().size(), false);

			for (const LightSource& source : stage.GetLightSources())
			{
				if (source.isOn)
				{
					TraceBeam(stage, source.position, math::DirectionFromDeg(source.angleDeg), out);
				}
			}
		}


		void LightTracer::TraceBeam(const Stage& stage, math::Vec2 origin, math::Vec2 direction, LightTraceResult& out) const
		{
			for (int bounce = 0; bounce <= maxBounces; ++bounce)
			{
				float nearestT = maxDistance;
				HitKind hitKind = HitKind::None;
				int hitIndex = -1;
				float t = 0.0f;

				// 地形ブロック
				for (const StageBlock& block : stage.GetBlocks())
				{
					if (RayIntersectRect(origin, direction, block.rect, t) && t < nearestT)
					{
						nearestT = t;
						hitKind = HitKind::Block;
					}
				}
				// 閉じている扉
				for (const Door& door : stage.GetDoors())
				{
					if (door.IsSolid() && RayIntersectRect(origin, direction, door.GetCurrentRect(), t) && t < nearestT)
					{
						nearestT = t;
						hitKind = HitKind::Block;
					}
				}
				// 受光器
				const std::vector<LightReceiver>& receivers = stage.GetLightReceivers();
				for (size_t i = 0; i < receivers.size(); ++i)
				{
					if (RayIntersectRect(origin, direction, receivers[i].rect, t) && t < nearestT)
					{
						nearestT = t;
						hitKind = HitKind::Receiver;
						hitIndex = static_cast<int>(i);
					}
				}
				// 反射鏡
				const std::vector<LightMirror>& mirrors = stage.GetLightMirrors();
				for (size_t i = 0; i < mirrors.size(); ++i)
				{
					if (RayIntersectSegment(origin, direction, mirrors[i].GetStart(), mirrors[i].GetEnd(), t) && t < nearestT)
					{
						nearestT = t;
						hitKind = HitKind::Mirror;
						hitIndex = static_cast<int>(i);
					}
				}

				// この線分を記録
				const math::Vec2 hitPoint = origin + direction * nearestT;
				out.segments.push_back({ origin, hitPoint, bounce });

				// 線分が通り抜けた光の足場を点灯
				const std::vector<LightPlatform>& platforms = stage.GetLightPlatforms();
				for (size_t i = 0; i < platforms.size(); ++i)
				{
					if (RayIntersectRect(origin, direction, platforms[i].rect, t) && t <= nearestT)
					{
						out.litPlatforms[i] = true;
					}
				}

				switch (hitKind)
				{
				case HitKind::Receiver:
					out.litReceivers[hitIndex] = true;
					return;

				case HitKind::Mirror:
				{
					const LightMirror& mirror = mirrors[hitIndex];
					math::Vec2 normal = mirror.GetNormal();
					const bool isFront = math::Dot(direction, normal) < 0.0f;
					if (!isFront)
					{
						if (!mirror.isDoubleSided)
						{
							// 裏側に当たった
							return;
						}
						normal = normal * -1.0f;
					}
					direction = math::Normalize(math::Reflect(direction, normal));
					origin = hitPoint + direction * RAY_EPSILON;
					break;
				}

				case HitKind::Block:
				case HitKind::None:
				default:
					// 止まった、または何にも当たらず最大距離まで進んだ
					return;
				}
			}
		}
	}
}
