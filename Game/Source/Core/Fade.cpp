/**
 * @file Fade.cpp
 * @brief 画面のフェード
 */
#include "stdafx.h"
#include "Fade.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace core
	{
		Fade* Fade::m_instance = nullptr;

		/** フェードに使う画像（真っ黒。小さい画像を画面の大きさに引き伸ばして出す） */
		static const char* const FADE_SPRITE_PATH = "Assets/sprite/fade.dds";


		Fade::Fade()
			: m_state(FadeState::None)
			, m_timer(0.0f)
			, m_duration(0.0f)
			, m_isFadeIn(false)
			, m_isFadeOut(false)
		{
			// 背景画像（画面全体）
			m_fadeRender.Init(FADE_SPRITE_PATH, static_cast<float>(FRAME_BUFFER_W), static_cast<float>(FRAME_BUFFER_H));
			m_fadeRender.Update();
		}


		Fade::~Fade()
		{}


		void Fade::Update()
		{
			if (m_state == FadeState::None)
			{
				return;
			}
			FadeProcess();
			m_fadeRender.Update();
		}


		void Fade::FadeProcess()
		{
			const float delta = g_gameTime->GetFrameDeltaTime();

			if (m_state == FadeState::FadeIn)
			{
				m_isFadeIn = true;
				m_timer -= delta;
				if (m_timer <= 0.0f)
				{
					m_timer = 0.0f;
					m_isFadeIn = false;
					m_state = FadeState::None;
				}
			}
			else if (m_state == FadeState::FadeOut)
			{
				m_isFadeOut = true;
				m_timer += delta;
				if (m_timer >= m_duration)
				{
					m_isFadeOut = false;
					m_timer = m_duration;
				}
			}
		}


		void Fade::FadeIn(const float duration)
		{
			m_state = FadeState::FadeIn;
			m_duration = duration;
			m_timer = duration;
		}


		void Fade::FadeOut(const float duration)
		{
			m_state = FadeState::FadeOut;
			m_duration = duration;
			m_timer = 0.0f;
		}


		float Fade::GetAlpha() const
		{
			if (m_state == FadeState::None)
			{
				return 0.0f;
			}
			// 時間が 0 のフェードは、フェードアウトならすぐ真っ暗、フェードインならすぐ明るい
			if (m_duration <= 0.0f)
			{
				return m_state == FadeState::FadeOut ? 1.0f : 0.0f;
			}
			return math::Clamp01(m_timer / m_duration);
		}


		void Fade::Render(RenderContext& rc)
		{
			if (m_state == FadeState::None)
			{
				return;
			}
			// 背景の描画（暗さを画像の透明度にする）
			m_fadeRender.SetMulColor({ 1.0f, 1.0f, 1.0f, GetAlpha() });
			m_fadeRender.Draw(rc);
		}
	}
}
