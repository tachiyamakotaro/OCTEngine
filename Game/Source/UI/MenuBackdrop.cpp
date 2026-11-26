/**
 * @file MenuBackdrop.cpp
 * @brief ウィンドウを開いている間の、後ろの画面
 */
#include "stdafx.h"
#include "MenuBackdrop.h"
#include "Source/Core/Math/MathUtil.h"


namespace app
{
	namespace ui
	{
		namespace
		{
			const char* const DIM_PATH = "Assets/sprite/fade.dds";
			/** 開ききったときの黒の濃さ（ぼかしがあるので、前の半透明の黒より薄くしている） */
			constexpr float DIM_ALPHA = 0.35f;
			/**
			 * スクリーンのぼかしの強さ（GaussianBlur の分散。大きいほどぼける）
			 * 0 より大きいとぼかした絵に切り替わるので、出始めは MIN から始める
			 */
			constexpr float BLUR_POWER_MIN = 1.0f;
			constexpr float BLUR_POWER_MAX = 12.0f;
			/** 開く・閉じるのにかかる時間（秒） */
			constexpr float FADE_TIME = 0.2f;
		}


		MenuBackdrop::MenuBackdrop()
			: m_dim()
			, m_rate(0.0f)
		{}


		MenuBackdrop::~MenuBackdrop()
		{
			// 次のシーンがぼけたままにならないように戻す
			RenderingEngine::GetInstance()->GetScreenBlurPower() = 0.0f;
		}


		void MenuBackdrop::Init()
		{
			m_dim.InitFullScreen(DIM_PATH);
			m_rate = 0.0f;
			Apply();
		}


		void MenuBackdrop::Update(const bool isShown)
		{
			const float target = isShown ? 1.0f : 0.0f;
			const float step = g_gameTime->GetFrameDeltaTime() / FADE_TIME;
			m_rate = math::MoveTowards(m_rate, target, step);
			Apply();
		}


		void MenuBackdrop::Draw(RenderContext& rc)
		{
			if (m_rate <= 0.0f)
			{
				return;
			}
			m_dim.Draw(rc);
		}


		void MenuBackdrop::Apply()
		{
			const float eased = math::SmoothStep(m_rate);
			// 0 のときはぼかさない（ぼかした絵に切り替えない）
			RenderingEngine::GetInstance()->GetScreenBlurPower() = m_rate > 0.0f ? math::Lerp(BLUR_POWER_MIN, BLUR_POWER_MAX, eased) : 0.0f;
			m_dim.Place(0.0f, 0.0f, 1.0f, Vector4(1.0f, 1.0f, 1.0f, DIM_ALPHA * eased));
		}
	}
}
