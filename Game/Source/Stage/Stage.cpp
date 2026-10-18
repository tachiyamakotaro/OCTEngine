/**
 * @file Stage.cpp
 * @brief ステージ（ギミックの入れ物・光の計算・奥の鏡）
 */
#include "stdafx.h"
#include "Stage.h"
#include <cmath>


namespace app
{
	namespace stage
	{
		void Stage::SetBackMirror(const bool enabled, const float planeZ, const float x, const float y, const float width, const float height)
		{
			m_backMirror.isEnabled = enabled;
			m_backMirror.planeZ = planeZ;
			m_backMirror.frame = math::Rect::FromMinSize(x, y, width, height);
		}


		void Stage::Clear()
		{
			m_gimmicks.Clear();
			m_lightResult.Clear();
			m_backMirror = BackMirror();
			m_playerStart = math::Vec2::Zero();
		}


		void Stage::ResetRuntimeState()
		{
			m_gimmicks.ResetRuntimeState();
			m_lightResult.Clear();
		}


		void Stage::Update(const float deltaTime)
		{
			static const std::vector<math::Rect> NO_BODIES;
			Update(deltaTime, NO_BODIES);
		}


		void Stage::Update(const float deltaTime, const std::vector<math::Rect>& bodies)
		{
			const GimmickUpdateContext context = { deltaTime, bodies, m_gimmicks, m_lightResult };

			// 1. 踏まれる・触れられる物（スイッチ）
			m_gimmicks.Update(UpdatePhase::Input, context);

			// 2. 光の経路を計算（今の扉・鏡の位置で）
			m_lightTracer.Trace(m_gimmicks, m_lightResult);

			// 3. 光を受ける物（受光器・光の足場）
			m_gimmicks.Update(UpdatePhase::Light, context);

			// 4. 信号を見て動く物（扉・霧・ランプ）
			m_gimmicks.Update(UpdatePhase::Signal, context);

			// 5. ほかのギミックの結果を見て動く物（霧に触れた透明の足場）
			m_gimmicks.Update(UpdatePhase::Late, context);
		}


		void Stage::GetMistVentPositions(const MistEmitter& emitter, std::vector<math::Vec2>& out) const
		{
			// 多すぎるとモデルや粒が増えすぎるので上限を決めておく
			constexpr int MAX_VENTS = 64;
			// 天井があるか調べる箱（吹き出し口の幅・すぐ上の薄い箱）
			constexpr float PROBE_HALF_WIDTH = 20.0f;
			constexpr float PROBE_HEIGHT = 5.0f;

			out.clear();
			const math::Rect& area = emitter.rect;
			const float areaWidth = area.Width();
			const float spacing = math::Max(emitter.ventSpacing, 1.0f);
			const int count = static_cast<int>(math::Clamp(std::floor(areaWidth / spacing), 1.0f, static_cast<float>(MAX_VENTS)));
			const float pitch = areaWidth / static_cast<float>(count);

			std::vector<math::Vec2> all;
			for (int i = 0; i < count; ++i)
			{
				const math::Vec2 position(area.min.x + pitch * (static_cast<float>(i) + 0.5f), area.max.y);
				all.push_back(position);
				const math::Rect probe = math::Rect::FromMinSize(position.x - PROBE_HALF_WIDTH, position.y, PROBE_HALF_WIDTH * 2.0f, PROBE_HEIGHT);
				if (OverlapsTerrain(probe))
				{
					out.push_back(position);
				}
			}
			if (out.empty())
			{
				out = all;
			}
		}


		const Checkpoint* Stage::FindCheckpoint(const math::Rect& body) const
		{
			for (const Checkpoint& checkpoint : m_gimmicks.View<Checkpoint>())
			{
				if (body.Overlaps(checkpoint.rect))
				{
					return &checkpoint;
				}
			}
			return nullptr;
		}


		const WarpZone* Stage::FindWarpZone(const math::Rect& body) const
		{
			const math::Vec2 center = (body.min + body.max) * 0.5f;
			for (const WarpZone& zone : m_gimmicks.View<WarpZone>())
			{
				const bool isInside = center.x > zone.rect.min.x && center.x < zone.rect.max.x
					&& center.y > zone.rect.min.y && center.y < zone.rect.max.y;
				if (!isInside)
				{
					continue;
				}
				if (!zone.disabledBySignal.empty() && IsSignalOn(zone.disabledBySignal))
				{
					continue;
				}
				return &zone;
			}
			return nullptr;
		}


		const Goal* Stage::FindGoal(const math::Rect& body) const
		{
			for (const Goal& goal : m_gimmicks.View<Goal>())
			{
				if (body.Overlaps(goal.rect))
				{
					return &goal;
				}
			}
			return nullptr;
		}


		void Stage::QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const
		{
			m_gimmicks.QueryOverlap(box, out);
		}


		bool Stage::AnyOverlap(const math::Rect& box) const
		{
			return m_gimmicks.AnyOverlap(box);
		}
	}
}
