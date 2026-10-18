/**
 * @file LightTracer.cpp
 * @brief 光の経路を計算する（レイを飛ばして鏡で反射させる）
 */
#include "stdafx.h"
#include "LightTracer.h"
#include <cfloat>
#include "Gimmick/GimmickManager.h"


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


		void LightTracer::Trace(const GimmickManager& gimmicks, LightTraceResult& out) const
		{
			out.Clear();
			for (const std::unique_ptr<GimmickBase>& gimmick : gimmicks.GetAll())
			{
				math::Vec2 origin;
				math::Vec2 direction;
				if (gimmick->GetEmittedRay(origin, direction))
				{
					TraceBeam(gimmicks, origin, direction, out);
				}
			}
		}


		void LightTracer::TraceBeam(const GimmickManager& gimmicks, math::Vec2 origin, math::Vec2 direction, LightTraceResult& out) const
		{
			for (int bounce = 0; bounce <= maxBounces; ++bounce)
			{
				// 光が止まる・反射する物のうち、一番近い物を探す（通り抜ける物は飛ばす）
				float nearestDistance = maxDistance;
				const GimmickBase* nearestGimmick = nullptr;
				LightHit nearestHit;
				for (const std::unique_ptr<GimmickBase>& gimmick : gimmicks.GetAll())
				{
					LightHit hit;
					if (gimmick->IntersectLight(origin, direction, hit)
						&& hit.response != LightResponse::PassThrough
						&& hit.distance < nearestDistance)
					{
						nearestDistance = hit.distance;
						nearestGimmick = gimmick.get();
						nearestHit = hit;
					}
				}

				// この線分を記録
				const math::Vec2 hitPoint = origin + direction * nearestDistance;
				out.segments.push_back({ origin, hitPoint, bounce });

				// 線分が通り抜けた物（光の足場など）を点灯
				for (const std::unique_ptr<GimmickBase>& gimmick : gimmicks.GetAll())
				{
					LightHit hit;
					if (gimmick->IntersectLight(origin, direction, hit)
						&& hit.response == LightResponse::PassThrough
						&& hit.distance <= nearestDistance)
					{
						out.litGimmicks.push_back(gimmick.get());
					}
				}

				if (nearestGimmick == nullptr)
				{
					// 何にも当たらず最大距離まで進んだ
					return;
				}

				switch (nearestHit.response)
				{
				case LightResponse::Absorb:
					out.litGimmicks.push_back(nearestGimmick);
					return;

				case LightResponse::Reflect:
					direction = math::Normalize(math::Reflect(direction, nearestHit.normal));
					origin = hitPoint + direction * RAY_EPSILON;
					break;

				case LightResponse::Stop:
				default:
					return;
				}
			}
		}
	}
}
