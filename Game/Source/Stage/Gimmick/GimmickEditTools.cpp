/**
 * @file GimmickEditTools.cpp
 * @brief ギミックがエディタ用の処理を書くための道具
 */
#include "stdafx.h"
#include "GimmickEditTools.h"
#include <algorithm>


namespace app
{
	namespace stage
	{
		/************************************/
		// EditorCanvas


		EditorCanvas::EditorCanvas(ImDrawList* drawList, const math::Vec2& viewCenter, const float zoom, const ImVec2& screenCenter)
			: m_drawList(drawList)
			, m_viewCenter(viewCenter)
			, m_zoom(zoom)
			, m_screenCenter(screenCenter)
		{}


		ImVec2 EditorCanvas::WorldToScreen(const math::Vec2& world) const
		{
			// 画面は下が +Y なので、Y は反転する
			return ImVec2(
				m_screenCenter.x + (world.x - m_viewCenter.x) * m_zoom,
				m_screenCenter.y - (world.y - m_viewCenter.y) * m_zoom);
		}


		void EditorCanvas::Rect(const math::Rect& rect, const ImU32 fillColor, const ImU32 outlineColor) const
		{
			const ImVec2 a = WorldToScreen({ rect.min.x, rect.max.y });
			const ImVec2 b = WorldToScreen({ rect.max.x, rect.min.y });
			if (fillColor != 0) { m_drawList->AddRectFilled(a, b, fillColor); }
			if (outlineColor != 0) { m_drawList->AddRect(a, b, outlineColor, 0.0f, 0, 2.0f); }
		}


		void EditorCanvas::Line(const math::Vec2& a, const math::Vec2& b, const ImU32 color, const float thickness) const
		{
			m_drawList->AddLine(WorldToScreen(a), WorldToScreen(b), color, thickness);
		}


		void EditorCanvas::Dot(const math::Vec2& center, const float radiusPixels, const ImU32 color) const
		{
			m_drawList->AddCircleFilled(WorldToScreen(center), radiusPixels, color);
		}


		void EditorCanvas::Label(const math::Vec2& world, const std::string& text) const
		{
			const ImVec2 p = WorldToScreen(world);
			m_drawList->AddText(ImVec2(p.x + 3.0f, p.y + 2.0f), COLOR_LABEL, text.c_str());
		}


		/************************************/
		// GimmickInspector


		bool GimmickInspector::EditRect(math::Rect& rect, const char* positionLabel, const char* sizeLabel)
		{
			float position[2] = { rect.min.x, rect.min.y };
			float size[2] = { rect.Width(), rect.Height() };
			const bool isPositionChanged = ImGui::DragFloat2(positionLabel, position, 1.0f, 0.0f, 0.0f, "%.1f");
			const bool isSizeChanged = ImGui::DragFloat2(sizeLabel, size, 1.0f, 1.0f, 100000.0f, "%.1f");
			if (!isPositionChanged && !isSizeChanged)
			{
				return false;
			}
			if (size[0] < 1.0f) { size[0] = 1.0f; }
			if (size[1] < 1.0f) { size[1] = 1.0f; }
			rect = math::Rect::FromMinSize(position[0], position[1], size[0], size[1]);
			return true;
		}


		void GimmickInspector::EditAngle(const char* label, float& angleDeg, const float stepDeg)
		{
			ImGui::PushID(label);
			if (ImGui::Button("-")) { angleDeg = math::NormalizeDeg(angleDeg - stepDeg); }
			ImGui::SameLine();
			if (ImGui::Button("+")) { angleDeg = math::NormalizeDeg(angleDeg + stepDeg); }
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-60.0f);
			if (ImGui::DragFloat(label, &angleDeg, 1.0f, 0.0f, 0.0f, "%.1f deg"))
			{
				angleDeg = math::NormalizeDeg(angleDeg);
			}
			ImGui::PopID();
		}


		void GimmickInspector::SignalCombo(const char* label, std::string& signal)
		{
			if (ImGui::BeginCombo(label, signal.empty() ? "(none)" : signal.c_str()))
			{
				if (ImGui::Selectable("(none)", signal.empty())) { signal.clear(); }
				for (const std::string& signalName : signalNames)
				{
					if (ImGui::Selectable(signalName.c_str(), signal == signalName)) { signal = signalName; }
				}
				ImGui::EndCombo();
			}
		}


		void GimmickInspector::SignalCheckboxes(std::vector<std::string>& signals)
		{
			for (const std::string& signalName : signalNames)
			{
				auto it = std::find(signals.begin(), signals.end(), signalName);
				bool isChecked = it != signals.end();
				if (ImGui::Checkbox(signalName.c_str(), &isChecked))
				{
					if (isChecked) { signals.push_back(signalName); }
					else { signals.erase(it); }
				}
			}
			if (signalNames.empty())
			{
				ImGui::TextDisabled("(no receivers / switches)");
			}
		}
	}
}
