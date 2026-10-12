/**
 * @file Stage.cpp
 * @brief ステージ（地形・光のギミック・スイッチと霧・鏡の世界）
 */
#include "stdafx.h"
#include "Stage.h"
#include <cmath>


namespace app
{
	namespace stage
	{
		StageBlock& Stage::AddBlock(const char* name, const float x, const float y, const float width, const float height)
		{
			m_blocks.push_back({ name, math::Rect::FromMinSize(x, y, width, height) });
			return m_blocks.back();
		}


		LightSource& Stage::AddLightSource(const char* name, const float x, const float y, const float angleDeg)
		{
			LightSource source;
			source.name = name;
			source.position = { x, y };
			source.angleDeg = angleDeg;
			m_lightSources.push_back(source);
			return m_lightSources.back();
		}


		LightMirror& Stage::AddLightMirror(const char* name, const float x, const float y, const float angleDeg, const float length)
		{
			LightMirror mirror;
			mirror.name = name;
			mirror.center = { x, y };
			mirror.angleDeg = angleDeg;
			mirror.length = length;
			m_lightMirrors.push_back(mirror);
			return m_lightMirrors.back();
		}


		LightReceiver& Stage::AddLightReceiver(const char* name, const float x, const float y, const float width, const float height)
		{
			LightReceiver receiver;
			receiver.name = name;
			receiver.rect = math::Rect::FromMinSize(x, y, width, height);
			m_lightReceivers.push_back(receiver);
			return m_lightReceivers.back();
		}


		Door& Stage::AddDoor(
			const char* name, const float x, const float y, const float width, const float height,
			const std::vector<std::string>& requiredReceivers)
		{
			Door door;
			door.name = name;
			door.closedRect = math::Rect::FromMinSize(x, y, width, height);
			door.requiredReceivers = requiredReceivers;
			m_doors.push_back(door);
			return m_doors.back();
		}


		LightPlatform& Stage::AddLightPlatform(const char* name, const float x, const float y, const float width, const float height)
		{
			LightPlatform platform;
			platform.name = name;
			platform.rect = math::Rect::FromMinSize(x, y, width, height);
			m_lightPlatforms.push_back(platform);
			return m_lightPlatforms.back();
		}


		FloorSwitch& Stage::AddFloorSwitch(const char* name, const float x, const float y, const float width, const float height)
		{
			FloorSwitch floorSwitch;
			floorSwitch.name = name;
			floorSwitch.rect = math::Rect::FromMinSize(x, y, width, height);
			m_floorSwitches.push_back(floorSwitch);
			return m_floorSwitches.back();
		}


		MistEmitter& Stage::AddMistEmitter(
			const char* name, const float x, const float y, const float width, const float height, const char* triggerSignal)
		{
			MistEmitter emitter;
			emitter.name = name;
			emitter.area = math::Rect::FromMinSize(x, y, width, height);
			emitter.triggerSignal = triggerSignal;
			m_mistEmitters.push_back(emitter);
			return m_mistEmitters.back();
		}


		MirrorPlatform& Stage::AddMirrorPlatform(const char* name, const float x, const float y, const float width, const float height)
		{
			MirrorPlatform platform;
			platform.name = name;
			platform.rect = math::Rect::FromMinSize(x, y, width, height);
			m_mirrorPlatforms.push_back(platform);
			return m_mirrorPlatforms.back();
		}


		Checkpoint& Stage::AddCheckpoint(const char* name, const float x, const float y, const float width, const float height)
		{
			m_checkpoints.push_back({ name, math::Rect::FromMinSize(x, y, width, height) });
			return m_checkpoints.back();
		}


		WarpZone& Stage::AddWarpZone(
			const char* name, const float x, const float y, const float width, const float height, const float destX, const float destY)
		{
			WarpZone zone;
			zone.name = name;
			zone.rect = math::Rect::FromMinSize(x, y, width, height);
			zone.destination = { destX, destY };
			m_warpZones.push_back(zone);
			return m_warpZones.back();
		}


		Indicator& Stage::AddIndicator(const char* name, const float x, const float y, const float width, const float height, const char* signal)
		{
			Indicator indicator;
			indicator.name = name;
			indicator.rect = math::Rect::FromMinSize(x, y, width, height);
			indicator.signal = signal;
			m_indicators.push_back(indicator);
			return m_indicators.back();
		}


		Goal& Stage::AddGoal(const char* name, const float x, const float y, const float width, const float height)
		{
			m_goals.push_back({ name, math::Rect::FromMinSize(x, y, width, height) });
			return m_goals.back();
		}


		void Stage::SetBackMirror(const bool enabled, const float planeZ, const float x, const float y, const float width, const float height)
		{
			m_backMirror.isEnabled = enabled;
			m_backMirror.planeZ = planeZ;
			m_backMirror.frame = math::Rect::FromMinSize(x, y, width, height);
		}


		void Stage::Clear()
		{
			m_blocks.clear();
			m_lightSources.clear();
			m_lightMirrors.clear();
			m_lightReceivers.clear();
			m_doors.clear();
			m_lightPlatforms.clear();
			m_floorSwitches.clear();
			m_mistEmitters.clear();
			m_mirrorPlatforms.clear();
			m_checkpoints.clear();
			m_warpZones.clear();
			m_indicators.clear();
			m_goals.clear();
			m_lightResult.Clear();
			m_backMirror = BackMirror();
		}


		void Stage::ResetRuntimeState()
		{
			for (LightReceiver& receiver : m_lightReceivers)
			{
				receiver.isLit = false;
			}
			for (Door& door : m_doors)
			{
				door.openAmount = 0.0f;
				door.isLatched = false;
			}
			for (LightPlatform& platform : m_lightPlatforms)
			{
				platform.isLit = false;
				platform.solidTimer = 0.0f;
			}
			for (FloorSwitch& floorSwitch : m_floorSwitches)
			{
				floorSwitch.isPressed = false;
			}
			for (MistEmitter& emitter : m_mistEmitters)
			{
				emitter.hasFired = false;
				emitter.isFiredThisFrame = false;
				emitter.timer = 0.0f;
			}
			for (MirrorPlatform& platform : m_mirrorPlatforms)
			{
				platform.isRevealed = false;
				platform.isRevealedThisFrame = false;
				platform.revealedTime = 0.0f;
			}
			for (Indicator& indicator : m_indicators)
			{
				indicator.isOn = false;
			}
			m_lightResult.Clear();
		}


		void Stage::Update(const float deltaTime)
		{
			static const std::vector<math::Rect> NO_BODIES;
			Update(deltaTime, NO_BODIES);
		}


		void Stage::Update(const float deltaTime, const std::vector<math::Rect>& bodies)
		{
			// 1. スイッチ（踏まれているか）
			UpdateFloorSwitches(bodies);

			// 2. 光の経路を計算（今の扉・鏡の位置で）
			m_lightTracer.Trace(*this, m_lightResult);

			// 3. 受光器
			for (size_t i = 0; i < m_lightReceivers.size(); ++i)
			{
				m_lightReceivers[i].isLit = m_lightResult.litReceivers[i];
			}

			// 4. 光の足場と扉
			UpdateLightPlatforms(deltaTime);
			UpdateDoors(deltaTime);

			// 5. 霧と鏡の足場
			UpdateMistEmitters(deltaTime);
			UpdateMirrorPlatforms(deltaTime);

			// 6. 合図のランプ（信号が空ならずっと消えたまま）
			for (Indicator& indicator : m_indicators)
			{
				indicator.isOn = !indicator.signal.empty() && IsSignalOn(indicator.signal);
			}
		}


		void Stage::UpdateFloorSwitches(const std::vector<math::Rect>& bodies)
		{
			for (FloorSwitch& floorSwitch : m_floorSwitches)
			{
				bool isStepped = false;
				for (const math::Rect& body : bodies)
				{
					if (body.Overlaps(floorSwitch.rect))
					{
						isStepped = true;
						break;
					}
				}
				// ラッチなら一度押したら押されたまま
				floorSwitch.isPressed = floorSwitch.isLatch ? (floorSwitch.isPressed || isStepped) : isStepped;
			}
		}


		void Stage::UpdateMistEmitters(const float deltaTime)
		{
			for (MistEmitter& emitter : m_mistEmitters)
			{
				emitter.isFiredThisFrame = false;
				if (!emitter.hasFired)
				{
					// 信号がONになったら一度だけ吹き出す
					if (IsSignalOn(emitter.triggerSignal))
					{
						emitter.hasFired = true;
						emitter.isFiredThisFrame = true;
						emitter.timer = 0.0f;
					}
					continue;
				}
				// 晴れたあとは時間を進めない（ずっと足し続けて桁あふれしないように）
				if (emitter.IsActive())
				{
					emitter.timer += deltaTime;
				}
			}
		}


		void Stage::UpdateMirrorPlatforms(const float deltaTime)
		{
			for (MirrorPlatform& platform : m_mirrorPlatforms)
			{
				platform.isRevealedThisFrame = false;
				if (platform.isRevealed)
				{
					// 露が残るので、一度見えたら見えたまま
					platform.revealedTime += deltaTime;
					continue;
				}
				for (const MistEmitter& emitter : m_mistEmitters)
				{
					// 霧が出ていて、霧の範囲の中で、霧の先端が足場の上面まで下りてきたら見える
					if (emitter.IsActive() && platform.rect.Overlaps(emitter.area) && emitter.GetFrontY() <= platform.rect.max.y)
					{
						platform.isRevealed = true;
						platform.isRevealedThisFrame = true;
						platform.revealedTime = 0.0f;
						break;
					}
				}
			}
		}


		void Stage::UpdateLightPlatforms(const float deltaTime)
		{
			for (size_t i = 0; i < m_lightPlatforms.size(); ++i)
			{
				LightPlatform& platform = m_lightPlatforms[i];
				platform.isLit = m_lightResult.litPlatforms[i];
				if (platform.isLit)
				{
					// 光が当たっている間は満タン、外れたら猶予時間だけ残る
					platform.solidTimer = platform.lingerTime > deltaTime ? platform.lingerTime : deltaTime;
				}
				else if (platform.solidTimer > 0.0f)
				{
					platform.solidTimer -= deltaTime;
				}
			}
		}


		void Stage::UpdateDoors(const float deltaTime)
		{
			for (Door& door : m_doors)
			{
				// つながった信号（受光器・スイッチ）が全部ONなら開く（つながりがなければ開かない）
				bool shouldOpen = !door.requiredReceivers.empty();
				for (const std::string& signalName : door.requiredReceivers)
				{
					if (!IsSignalOn(signalName))
					{
						shouldOpen = false;
						break;
					}
				}
				if (door.isLatched)
				{
					shouldOpen = true;
				}

				const float target = shouldOpen ? 1.0f : 0.0f;
				door.openAmount = math::MoveTowards(door.openAmount, target, door.openSpeed * deltaTime);

				if (door.isLatch && door.openAmount >= 1.0f)
				{
					door.isLatched = true;
				}
			}
		}


		bool Stage::IsReceiverLit(const std::string& name) const
		{
			const int index = FindReceiverIndex(name);
			return index >= 0 && m_lightReceivers[index].isLit;
		}


		int Stage::FindReceiverIndex(const std::string& name) const
		{
			for (size_t i = 0; i < m_lightReceivers.size(); ++i)
			{
				if (m_lightReceivers[i].name == name)
				{
					return static_cast<int>(i);
				}
			}
			return -1;
		}


		bool Stage::IsSwitchPressed(const std::string& name) const
		{
			for (const FloorSwitch& floorSwitch : m_floorSwitches)
			{
				if (floorSwitch.name == name)
				{
					return floorSwitch.isPressed;
				}
			}
			return false;
		}


		bool Stage::IsSignalOn(const std::string& name) const
		{
			return IsReceiverLit(name) || IsSwitchPressed(name);
		}


		void Stage::GetMistVentPositions(const MistEmitter& emitter, std::vector<math::Vec2>& out) const
		{
			// 多すぎるとモデルや粒が増えすぎるので上限を決めておく
			constexpr int MAX_VENTS = 64;
			// 天井があるか調べる箱（吹き出し口の幅・すぐ上の薄い箱）
			constexpr float PROBE_HALF_WIDTH = 20.0f;
			constexpr float PROBE_HEIGHT = 5.0f;

			out.clear();
			const math::Rect& area = emitter.area;
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
			for (const Checkpoint& checkpoint : m_checkpoints)
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
			for (const WarpZone& zone : m_warpZones)
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
			for (const Goal& goal : m_goals)
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
			for (const StageBlock& block : m_blocks)
			{
				if (box.Overlaps(block.rect)) { out.push_back(block.rect); }
			}
			for (const Door& door : m_doors)
			{
				if (!door.IsSolid()) { continue; }
				const math::Rect rect = door.GetCurrentRect();
				if (box.Overlaps(rect)) { out.push_back(rect); }
			}
			for (const LightPlatform& platform : m_lightPlatforms)
			{
				if (platform.IsSolid() && box.Overlaps(platform.rect)) { out.push_back(platform.rect); }
			}
			for (const MirrorPlatform& platform : m_mirrorPlatforms)
			{
				if (platform.IsSolid() && box.Overlaps(platform.rect)) { out.push_back(platform.rect); }
			}
		}


		bool Stage::AnyOverlap(const math::Rect& box) const
		{
			if (OverlapsTerrain(box))
			{
				return true;
			}
			for (const LightPlatform& platform : m_lightPlatforms)
			{
				if (platform.IsSolid() && box.Overlaps(platform.rect)) { return true; }
			}
			return false;
		}


		bool Stage::OverlapsTerrain(const math::Rect& box) const
		{
			for (const StageBlock& block : m_blocks)
			{
				if (box.Overlaps(block.rect)) { return true; }
			}
			for (const Door& door : m_doors)
			{
				if (door.IsSolid() && box.Overlaps(door.GetCurrentRect())) { return true; }
			}
			for (const MirrorPlatform& platform : m_mirrorPlatforms)
			{
				if (platform.IsSolid() && box.Overlaps(platform.rect)) { return true; }
			}
			return false;
		}


	}
}
