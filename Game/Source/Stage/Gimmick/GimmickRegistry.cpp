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
				{ "Moving Block", "MovingBlock", [](const math::Vec2& p) { return std::make_unique<MovingBlock>("", p.x, p.y, 200.0f, 30.0f); } },
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
