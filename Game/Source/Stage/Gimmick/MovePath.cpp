/**
 * @file MovePath.cpp
 * @brief 「始点と終点の間を動く」動きの部品
 */
#include "stdafx.h"
#include "MovePath.h"
#include "GimmickArchive.h"
#include "GimmickEditTools.h"


namespace app
{
	namespace stage
	{
		void MovePath::Update(const bool isPowered, const float deltaTime)
		{
			// 動いてはいけない間（光が当たっていない間）は、その場で止まる（端の待ち時間も進めない）
			if (!isPowered)
			{
				return;
			}
			const float length = GetLength();
			if (length <= 0.0f || speed <= 0.0f)
			{
				return;
			}

			// 端で止まっている
			if (waitTimer > 0.0f)
			{
				waitTimer -= deltaTime;
				return;
			}

			// 向かっている端へ近づける（progress は 0〜1 なので、距離を道のりの長さで割る）
			const float target = direction > 0.0f ? 1.0f : 0.0f;
			progress = math::MoveTowards(progress, target, speed * deltaTime / length);
			if (progress != target)
			{
				return;
			}

			// 端に着いた
			if (mode == Mode::OneWay)
			{
				// 終点で止まる（向きは終点のまま）
				return;
			}
			direction = -direction;
			waitTimer = waitTime;
		}


		void MovePath::ResetRuntimeState()
		{
			progress = 0.0f;
			direction = 1.0f;
			waitTimer = 0.0f;
		}


		void MovePath::Rotate90(const int quarterTurns)
		{
			// +1 で反時計回りに90度：(x, y) → (-y, x)
			const int turns = ((quarterTurns % 4) + 4) % 4;
			for (int i = 0; i < turns; ++i)
			{
				offset = math::Vec2(-offset.y, offset.x);
			}
		}


		void MovePath::DrawInspector()
		{
			static const char* MODE_NAMES[] = { "Ping-Pong", "One Way" };

			ImGui::SeparatorText("Move");
			float values[2] = { offset.x, offset.y };
			if (ImGui::DragFloat2("Move Offset", values, 1.0f, 0.0f, 0.0f, "%.1f"))
			{
				offset = math::Vec2(values[0], values[1]);
			}
			ImGui::SetItemTooltip("How far the end point is from the placed position.");
			ImGui::DragFloat("Move Speed", &speed, 1.0f, 1.0f, 5000.0f, "%.0f");
			int modeIndex = static_cast<int>(mode);
			if (ImGui::Combo("Move Mode", &modeIndex, MODE_NAMES, static_cast<int>(Mode::Num)))
			{
				mode = static_cast<Mode>(modeIndex);
			}
			if (mode == Mode::PingPong)
			{
				ImGui::DragFloat("Wait at Ends", &waitTime, 0.05f, 0.0f, 10.0f, "%.2f s");
			}
			ImGui::Text("Progress: %.0f%%", progress * 100.0f);
		}


		void MovePath::Serialize(GimmickArchive& archive)
		{
			archive.Field("moveOffset", offset);
			archive.Field("moveSpeed", speed);
			// enum はそのまま保存できないので、数字にして保存する（0=往復, 1=片道）
			int modeValue = static_cast<int>(mode);
			archive.Field("moveMode", modeValue);
			if (archive.IsReading())
			{
				const bool isValid = modeValue >= 0 && modeValue < static_cast<int>(Mode::Num);
				mode = isValid ? static_cast<Mode>(modeValue) : Mode::PingPong;
			}
			archive.Field("moveWaitTime", waitTime);
		}


		void MovePath::DrawEditor(const EditorCanvas& canvas, const math::Vec2& startCenter, const unsigned int color) const
		{
			const math::Vec2 endCenter = startCenter + offset;
			canvas.Line(startCenter, endCenter, color, 1.5f);
			canvas.Dot(startCenter, 3.0f, color);
			canvas.Dot(endCenter, 4.0f, color);
		}
	}
}
