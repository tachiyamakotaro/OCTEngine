/**
 * @file GimmickRegistry.cpp
 * @brief エディタで置けるギミックの一覧
 */
#include "stdafx.h"
#include "GimmickRegistry.h"
#include "Source/Stage/StageObjects.h"
#include <cstring>


namespace app
{
	namespace stage
	{
		GimmickRegistryEntry::GimmickRegistryEntry(const char* buttonLabel, const char* namePrefix, CreateFunction create)
			: buttonLabel(buttonLabel)
			, namePrefix(namePrefix)
			, create(std::move(create))
			, typeName(this->create(math::Vec2::Zero())->GetTypeName())
		{}


		const std::vector<GimmickRegistryEntry>& GetGimmickRegistry()
		{
			// { 追加ボタンの表示, 名前の頭, [](const math::Vec2& p) { return 見取り図の中心 p に置くギミック; } }
			// 新しいギミックを作ったら、ここに1行足す
			static const std::vector<GimmickRegistryEntry> REGISTRY = {
				{ "Block", "Block", [](const math::Vec2& p) { return std::make_unique<StageBlock>("", p.x, p.y, 200.0f, 50.0f); } },
				{ "Emit Ray", "Lamp", [](const math::Vec2& p) { return std::make_unique<EmitRay>("", p.x, p.y, 0.0f); } },
				{ "Light Mirror", "Mirror", [](const math::Vec2& p) { return std::make_unique<LightMirror>("", p.x, p.y, 45.0f, 80.0f); } },
				{ "Receiver", "Receiver", [](const math::Vec2& p) { return std::make_unique<LightReceiver>("", p.x, p.y, 40.0f, 40.0f); } },
				{ "Door", "Door", [](const math::Vec2& p) { return std::make_unique<Door>("", p.x, p.y, 40.0f, 200.0f); } },
				{ "Light Platform", "LightPlatform", [](const math::Vec2& p) { return std::make_unique<LightPlatform>("", p.x, p.y, 150.0f, 25.0f); } },
				{ "Switch", "Switch", [](const math::Vec2& p) { return std::make_unique<FloorSwitch>("", p.x, p.y, 70.0f, 8.0f); } },
				{ "Mist", "Mist", [](const math::Vec2& p) { return std::make_unique<MistEmitter>("", p.x, p.y, 600.0f, 600.0f, ""); } },
				{ "Invisible Floor", "InvisibleFloor", [](const math::Vec2& p) { return std::make_unique<InvisibleFloor>("", p.x, p.y, 150.0f, 25.0f); } },
				{ "Checkpoint", "Checkpoint", [](const math::Vec2& p) { return std::make_unique<Checkpoint>("", p.x, p.y, 80.0f, 200.0f); } },
				{ "Warp", "Warp", [](const math::Vec2& p) { return std::make_unique<WarpZone>("", p.x, p.y, 150.0f, 200.0f, p.x - 300.0f, p.y); } },
				{ "Lamp", "Lamp", [](const math::Vec2& p) { return std::make_unique<Indicator>("", p.x, p.y, 40.0f, 30.0f, ""); } },
				{ "Goal", "Goal", [](const math::Vec2& p) { return std::make_unique<Goal>("", p.x, p.y, 150.0f, 250.0f); } },
				// 暗がりと、光で照らすと見える足場
				{ "Dark Zone", "Dark", [](const math::Vec2& p) { return std::make_unique<DarkZone>("", p.x, p.y, 800.0f, 600.0f); } },
				{ "Hidden Floor", "HiddenFloor", [](const math::Vec2& p) { return std::make_unique<HiddenFloor>("", p.x, p.y, 150.0f, 25.0f); } },
				// 光の現象のギミック（氷・日光・日よけ板）
				{ "Ice (Melt)", "IceWall", [](const math::Vec2& p) { return std::make_unique<IceBlock>("", p.x, p.y, 80.0f, 300.0f, IceKind::Melt); } },
				{ "Ice (Keep)", "IceFloor", [](const math::Vec2& p)
					{
						auto ice = std::make_unique<IceBlock>("", p.x, p.y, 200.0f, 40.0f, IceKind::Keep);
						ice->meltTime = 5.0f;
						return ice;
					} },
				{ "Sun Light", "Sun", [](const math::Vec2& p) { return std::make_unique<SunLight>("", p.x, p.y, 400.0f, -60.0f); } },
				{ "Shade Board", "Shade", [](const math::Vec2& p) { return std::make_unique<ShadeBoard>("", p.x, p.y, 300.0f, 20.0f); } },
			};
			return REGISTRY;
		}


		const GimmickRegistryEntry* FindGimmickRegistryEntry(const char* typeName)
		{
			for (const GimmickRegistryEntry& entry : GetGimmickRegistry())
			{
				if (std::strcmp(entry.typeName, typeName) == 0)
				{
					return &entry;
				}
			}
			return nullptr;
		}
	}
}
