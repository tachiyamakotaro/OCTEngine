/**
 * @file MistEmitter.cpp
 * @brief 霧の発生装置
 */
#include "stdafx.h"
#include "MistEmitter.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"
#include "GimmickManager.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			const ImU32 EDITOR_COLOR_FILL = IM_COL32(225, 235, 245, 14);
			const ImU32 EDITOR_COLOR_LINE = IM_COL32(225, 235, 245, 120);
		}


		MistEmitter::MistEmitter(
			const char* name, const float x, const float y, const float width, const float height, const char* initialTriggerSignal)
			: RectGimmick(name, x, y, width, height)
			, triggerSignal(initialTriggerSignal != nullptr ? initialTriggerSignal : "")
		{}


		void MistEmitter::Update(const GimmickUpdateContext& context)
		{
			isFiredThisFrame = false;
			if (!hasFired)
			{
				// 信号がONになったら一度だけ吹き出す
				if (context.gimmicks.IsSignalOn(triggerSignal))
				{
					hasFired = true;
					isFiredThisFrame = true;
					timer = 0.0f;
				}
				return;
			}
			// 晴れたあとは時間を進めない（ずっと足し続けて桁あふれしないように）
			if (IsActive())
			{
				timer += context.deltaTime;
			}
		}


		void MistEmitter::ResetRuntimeState()
		{
			hasFired = false;
			isFiredThisFrame = false;
			timer = 0.0f;
		}


		float MistEmitter::GetDensity() const
		{
			if (!IsActive()) { return 0.0f; }
			constexpr float FADE_IN_TIME = 0.3f;
			const float fadeIn = math::Clamp01(timer / FADE_IN_TIME);
			const float fadeOut = timer <= duration ? 1.0f : 1.0f - math::Clamp01((timer - duration) / math::Max(fadeTime, 0.001f));
			return fadeIn * fadeOut;
		}


		void MistEmitter::RenameSignal(const std::string& oldName, const std::string& newName)
		{
			if (triggerSignal == oldName) { triggerSignal = newName; }
		}


		void MistEmitter::RemoveSignal(const std::string& signalName)
		{
			if (triggerSignal == signalName) { triggerSignal.clear(); }
		}


		bool MistEmitter::HitTest(const math::Vec2& point, const float pickDistance) const
		{
			const bool isInsideOuter = point.x >= rect.min.x - pickDistance && point.x <= rect.max.x + pickDistance
				&& point.y >= rect.min.y - pickDistance && point.y <= rect.max.y + pickDistance;
			const bool isInsideInner = point.x > rect.min.x + pickDistance && point.x < rect.max.x - pickDistance
				&& point.y > rect.min.y + pickDistance && point.y < rect.max.y - pickDistance;
			return isInsideOuter && !isInsideInner;
		}


		void MistEmitter::DrawEditor(const EditorCanvas& canvas, const bool isSelected) const
		{
			// 薄い塗りと枠。先端の線は霧が出ている間だけ
			canvas.Rect(rect, EDITOR_COLOR_FILL, EditorCanvas::Pick(isSelected, EDITOR_COLOR_LINE));
			if (IsActive())
			{
				const float frontY = GetFrontY();
				canvas.Line({ rect.min.x, frontY }, { rect.max.x, frontY }, EDITOR_COLOR_LINE, 2.0f);
			}
			canvas.Label({ rect.min.x, rect.max.y }, name + (triggerSignal.empty() ? "" : " <- " + triggerSignal));
		}


		void MistEmitter::DrawInspector(GimmickInspector& inspector)
		{
			RectGimmick::DrawInspector(inspector);
			ImGui::SetItemTooltip("The mist spreads down from the top of this area (the ceiling).");
			inspector.SignalCombo("Trigger", triggerSignal);
			ImGui::DragFloat("Fall Speed", &fallSpeed, 5.0f, 1.0f, 10000.0f, "%.0f");
			ImGui::DragFloat("Duration", &duration, 0.05f, 0.0f, 60.0f, "%.2f s");
			ImGui::DragFloat("Fade Time", &fadeTime, 0.05f, 0.0f, 60.0f, "%.2f s");
			ImGui::DragFloat("Vent Spacing", &ventSpacing, 1.0f, 20.0f, 5000.0f, "%.0f");
			ImGui::Text("Fired: %s  Time: %.2f  Density: %.2f", hasFired ? "YES" : "no", timer, GetDensity());
			if (ImGui::SmallButton("Fire now"))
			{
				hasFired = true;
				timer = 0.0f;
			}
			ImGui::SetItemTooltip("Blow the mist without the trigger (for testing). Reset Runtime to undo.");
		}


		void MistEmitter::Serialize(GimmickArchive& archive)
		{
			RectGimmick::Serialize(archive);
			archive.Field("triggerSignal", triggerSignal);
			archive.Field("fallSpeed", fallSpeed);
			archive.Field("duration", duration);
			archive.Field("fadeTime", fadeTime);
			archive.Field("ventSpacing", ventSpacing);
		}
	}
}
