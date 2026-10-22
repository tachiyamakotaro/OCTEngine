/**
 * @file StageTimer.cpp
 * @brief ステージの時間制限
 */
#include "stdafx.h"
#include "StageTimer.h"
#include "TimerParameter.h"
#include "imgui.h"
#include <cmath>


namespace app
{
	namespace timer
	{
		StageTimer::StageTimer()
			: m_limit(0.0f)
			, m_remaining(0.0f)
			, m_isRunning(false)
			, m_isPaused(false)
			, m_isTimeUp(false)
			, m_isTimeUpThisFrame(false)
			, m_isSecondChangedThisFrame(false)
		{}


		void StageTimer::Start(const float limitSeconds)
		{
			m_limit = limitSeconds > 0.0f ? limitSeconds : 0.0f;
			Restart();
		}


		void StageTimer::Restart()
		{
			m_remaining = m_limit;
			m_isRunning = true;
			m_isTimeUp = false;
			m_isTimeUpThisFrame = false;
			m_isSecondChangedThisFrame = false;
		}


		void StageTimer::Update(const float deltaTime)
		{
			// 「このフレームで〇〇した」は毎フレーム消す（止まっている間も）
			m_isTimeUpThisFrame = false;
			m_isSecondChangedThisFrame = false;

			if (!IsEnabled() || !m_isRunning || m_isPaused || m_isTimeUp)
			{
				return;
			}

			const int beforeSeconds = GetDisplaySeconds();
			m_remaining -= deltaTime;
			if (m_remaining < 0.0f)
			{
				m_remaining = 0.0f;
			}
			m_isSecondChangedThisFrame = GetDisplaySeconds() != beforeSeconds;

			CheckTimeUp();
		}


		void StageTimer::AddTime(const float seconds)
		{
			if (!IsEnabled() || m_isTimeUp)
			{
				return;
			}
			m_remaining += seconds;
			if (m_remaining < 0.0f)
			{
				m_remaining = 0.0f;
			}
			// 制限時間より多くなってもよい（ゲージは満タンのまま）
		}


		void StageTimer::SetRemaining(const float seconds)
		{
			if (!IsEnabled() || m_isTimeUp)
			{
				return;
			}
			// 0 にしたときは、次の Update で時間切れになる
			m_remaining = seconds > 0.0f ? seconds : 0.0f;
		}


		float StageTimer::GetRemainingRate() const
		{
			if (!IsEnabled())
			{
				return 1.0f;
			}
			const float rate = m_remaining / m_limit;
			return rate < 0.0f ? 0.0f : (rate > 1.0f ? 1.0f : rate);
		}


		int StageTimer::GetDisplaySeconds() const
		{
			// 切り上げ：残り 0.3 秒なら 1 を出す（0 を出すのは時間切れのときだけ）
			return static_cast<int>(std::ceil(m_remaining));
		}


		TimeLevel StageTimer::GetLevel() const
		{
			if (!IsEnabled())
			{
				return TimeLevel::Normal;
			}
			if (m_isTimeUp)
			{
				return TimeLevel::TimeUp;
			}
			// 表示の秒数で決める（数字が 30 に変わった瞬間に色が変わるように）
			const float shown = static_cast<float>(GetDisplaySeconds());
			if (shown <= DANGER_TIME)
			{
				return TimeLevel::Danger;
			}
			if (shown <= CAUTION_TIME)
			{
				return TimeLevel::Caution;
			}
			return TimeLevel::Normal;
		}


		void StageTimer::DrawSettingsUI()
		{
			// ほかの調整項目とラベルがかぶっても混ざらないようにする
			ImGui::PushID("StageTimer");

			if (!IsEnabled())
			{
				ImGui::TextDisabled("No time limit on this stage.");
				ImGui::PopID();
				return;
			}

			// 時間切れの演出中は触れない（やり直しはシーンが自動で始める）
			ImGui::BeginDisabled(m_isTimeUp);

			// 止める・動かす（止めている間も +10s などのボタンは使える）
			if (ImGui::Button(m_isPaused ? "Resume Timer" : "Stop Timer"))
			{
				m_isPaused = !m_isPaused;
			}
			ImGui::SetItemTooltip("Debug only. The timer runs again when the debug UI (F4) is turned off.");
			if (m_isPaused)
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "STOPPED");
			}

			// 今の状態
			static const char* const LEVEL_NAMES[] = { "Normal", "Caution", "Danger", "TimeUp" };
			ImGui::Text("Remaining: %.2f / %.0f s  (%s)", m_remaining, m_limit, LEVEL_NAMES[static_cast<int>(GetLevel())]);
			ImGui::ProgressBar(GetRemainingRate(), ImVec2(-1.0f, 0.0f));

			// 時間を足す・引く
			if (ImGui::SmallButton("+10s"))
			{
				AddTime(10.0f);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("-10s"))
			{
				AddTime(-10.0f);
			}
			// 注意・危険・時間切れの見た目をすぐ確かめる（CAUTION_TIME / DANGER_TIME の1秒前にする）
			ImGui::SameLine();
			if (ImGui::SmallButton("Caution-1s"))
			{
				SetRemaining(CAUTION_TIME + 1.0f);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Danger-1s"))
			{
				SetRemaining(DANGER_TIME + 1.0f);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Time Up"))
			{
				SetRemaining(0.0f);
			}

			// 満タンに戻す
			if (ImGui::Button("Restart Timer"))
			{
				Restart();
			}
			ImGui::SetItemTooltip("Refill the timer. The player stays where it is.");

			ImGui::EndDisabled();
			ImGui::PopID();
		}


		void StageTimer::CheckTimeUp()
		{
			if (!m_isTimeUp && m_remaining <= 0.0f)
			{
				m_isTimeUp = true;
				m_isTimeUpThisFrame = true;
			}
		}
	}
}
