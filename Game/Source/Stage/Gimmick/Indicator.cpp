/**
 * @file Indicator.cpp
 * @brief 合図のランプ
 */
#include "stdafx.h"
#include "Indicator.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "GimmickManager.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_OFF = IM_COL32(90, 90, 100, 255);
			const ImU32 EDITOR_COLOR_ON = IM_COL32(120, 255, 140, 255);
		}


		Indicator::Indicator(const char* name, const float x, const float y, const float width, const float height, const char* initialSignal)
			: RectGimmick(name, x, y, width, height)
			, signal(initialSignal != nullptr ? initialSignal : "")
		{}


		void Indicator::Update(const GimmickUpdateContext& context)
		{
			// 信号が空ならずっと消えたまま（IsSignalOn は空の名前に false を返す）
			isOn = context.gimmicks.IsSignalOn(signal);
		}


		void Indicator::RenameSignal(const std::string& oldName, const std::string& newName)
		{
			if (signal == oldName) { signal = newName; }
		}


		void Indicator::RemoveSignal(const std::string& signalName)
		{
			if (signal == signalName) { signal.clear(); }
		}


		void Indicator::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			canvas.Rect(rect, isOn ? EDITOR_COLOR_ON : EDITOR_COLOR_OFF, isSelected ? EditorCanvas::COLOR_SELECTED : 0);
			canvas.Label({ rect.min.x, rect.max.y }, name + (signal.empty() ? "" : " <- " + signal));
		}


		void Indicator::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			inspector.SignalCombo("Signal", signal);
			ImGui::Text("On: %s", isOn ? "YES" : "no");
			ImGui::TextDisabled("(none) = a dummy lamp that never lights.");
		}


		void Indicator::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("signal", signal);
		}
	}
}
