/**
 * @file Door.cpp
 * @brief 扉
 */
#include "stdafx.h"
#include "Door.h"
#include <algorithm>
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "GimmickManager.h"
#include "Source/Stage/LightTracer.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_DOOR = IM_COL32(170, 110, 60, 255);
		}


		Door::Door(const char* name, const float x, const float y, const float width, const float height)
			: RectGimmick(name, x, y, width, height)
		{}


		void Door::Update(const GimmickUpdateContext& context)
		{
			// つながった信号（受光器・スイッチ）が全部ONなら開く（つながりがなければ開かない）
			bool shouldOpen = !requiredReceivers.empty();
			for (const std::string& signalName : requiredReceivers)
			{
				if (!context.gimmicks.IsSignalOn(signalName))
				{
					shouldOpen = false;
					break;
				}
			}
			if (isLatched)
			{
				shouldOpen = true;
			}

			const float target = shouldOpen ? 1.0f : 0.0f;
			openAmount = math::MoveTowards(openAmount, target, openSpeed * context.deltaTime);

			if (isLatch && openAmount >= 1.0f)
			{
				isLatched = true;
			}
		}


		void Door::ResetRuntimeState()
		{
			openAmount = 0.0f;
			isLatched = false;
		}


		CollisionKind Door::GetCollision(math::Rect& outRect) const
		{
			if (!IsSolid())
			{
				return CollisionKind::None;
			}
			outRect = GetCurrentRect();
			return CollisionKind::Terrain;
		}


		bool Door::IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const
		{
			// 閉じている扉は光を止める
			if (!IsSolid() || !RayIntersectRect(origin, direction, GetCurrentRect(), outHit.distance))
			{
				return false;
			}
			outHit.response = LightResponse::Stop;
			return true;
		}


		void Door::RenameSignal(const std::string& oldName, const std::string& newName)
		{
			for (std::string& required : requiredReceivers)
			{
				if (required == oldName) { required = newName; }
			}
		}


		void Door::RemoveSignal(const std::string& signalName)
		{
			requiredReceivers.erase(std::remove(requiredReceivers.begin(), requiredReceivers.end(), signalName), requiredReceivers.end());
		}


		void Door::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 閉じている範囲を枠で、今の範囲を塗りで
			if (IsSolid()) { canvas.Rect(GetCurrentRect(), EDITOR_COLOR_DOOR, 0); }
			canvas.Rect(rect, 0, EditorCanvas::Pick(isSelected, EDITOR_COLOR_DOOR));
			canvas.Label({ rect.min.x, rect.max.y }, name);
		}


		void Door::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::DragFloat("Open Speed", &openSpeed, 0.05f, 0.05f, 20.0f, "%.2f");
			ImGui::Checkbox("Latch (stay open)", &isLatch);
			ImGui::Text("Open: %.0f%%", openAmount * 100.0f);
			ImGui::TextUnformatted("Required signals (receivers / switches):");
			inspector.SignalCheckboxes(requiredReceivers);
		}


		void Door::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("requiredReceivers", requiredReceivers);
			archive.Field("openSpeed", openSpeed);
			archive.Field("isLatch", isLatch);
		}
	}
}
