/**
 * @file ResultUI.cpp
 * @brief 宝物の間の 2D の表示
 */
#include "stdafx.h"
#include "ResultUI.h"
#include "Source/Result/CelebrationTimeline.h"
#include "Source/Core/Math/MathUtil.h"
#include <cmath>


namespace
{
	// ---- 画像（パスと、表示するときの縦横の比を決める大きさ） ----
	const char* const LOGO_PATH = "Assets/sprite/result/gameclear_logo.dds";
	constexpr float LOGO_PIXEL_W = 1360.0f;
	constexpr float LOGO_PIXEL_H = 680.0f;
	const char* const CONGRATULATIONS_PATH = "Assets/sprite/result/congratulations.dds";
	constexpr float CONGRATULATIONS_PIXEL_W = 1024.0f;
	constexpr float CONGRATULATIONS_PIXEL_H = 128.0f;
	/** A ボタンはポーズ画面と同じ画像（絵のまわりで切ってある） */
	const char* const BUTTON_A_PATH = "Assets/sprite/pause/button_a.dds";
	constexpr float BUTTON_A_PIXEL_W = 104.0f;
	constexpr float BUTTON_A_PIXEL_H = 102.0f;
	/** TITLE の文字（ポーズ画面の TITLE と同じ画像。縦横の比もポーズ画面と同じ） */
	const char* const LABEL_TITLE_PATH = "Assets/sprite/result/label_title.dds";
	constexpr float LABEL_TITLE_PIXEL_W = 1050.0f;
	constexpr float LABEL_TITLE_PIXEL_H = 306.0f;

	// ---- 配置（画面の真ん中が原点、上が +Y） ----
	/** ロゴと CONGRATULATIONS! は画面の左上（右には宝箱と宝石、左下にはプレイヤーがいる） */
	constexpr float LOGO_X = -400.0f;
	constexpr float LOGO_Y = 300.0f;
	constexpr float LOGO_HEIGHT = 330.0f;
	constexpr float CONGRATULATIONS_Y = 95.0f;
	constexpr float CONGRATULATIONS_HEIGHT = 56.0f;
	/** 右下の案内（A の絵の真ん中と、TITLE の文字の真ん中） */
	constexpr float BUTTON_A_X = 640.0f;
	constexpr float PROMPT_Y = -450.0f;
	constexpr float BUTTON_A_SIZE = 52.0f;
	constexpr float LABEL_TITLE_X = 760.0f;
	constexpr float LABEL_TITLE_HEIGHT = 44.0f;

	// ---- 動き ----
	/** ロゴが出始めるときの大きさ（ここから 1 へ弾むように縮む） */
	constexpr float LOGO_START_SCALE = 1.8f;
	/** CONGRATULATIONS! がロゴより遅れて出る割合（ロゴの出具合がこの値を超えたら出始める） */
	constexpr float CONGRATULATIONS_DELAY = 0.5f;
	/** 案内の点滅の速さ（1秒に何回）と、いちばん薄いときの濃さ */
	constexpr float PROMPT_BLINK_SPEED = 1.2f;
	constexpr float PROMPT_MIN_ALPHA = 0.35f;
	/** 案内が出きるまでの時間 */
	constexpr float PROMPT_FADE_TIME = 0.4f;
	constexpr float TWO_PI = 6.28318530718f;

	/**
	 * @brief 少し行きすぎてから戻る動き（0〜1 → 0〜1。途中で 1 を少し超える）
	 */
	float EaseOutBack(const float t)
	{
		const float c1 = 1.70158f;
		const float c3 = c1 + 1.0f;
		const float u = t - 1.0f;
		return 1.0f + c3 * u * u * u + c1 * u * u;
	}
}


namespace app
{
	namespace result
	{
		void ResultUI::Init()
		{
			m_logo.Init(LOGO_PATH, LOGO_PIXEL_W, LOGO_PIXEL_H, LOGO_HEIGHT);
			m_congratulations.Init(CONGRATULATIONS_PATH, CONGRATULATIONS_PIXEL_W, CONGRATULATIONS_PIXEL_H, CONGRATULATIONS_HEIGHT);
			m_buttonA.Init(BUTTON_A_PATH, BUTTON_A_PIXEL_W, BUTTON_A_PIXEL_H, BUTTON_A_SIZE);
			m_labelTitle.Init(LABEL_TITLE_PATH, LABEL_TITLE_PIXEL_W, LABEL_TITLE_PIXEL_H, LABEL_TITLE_HEIGHT);
			m_isLogoVisible = false;
			m_isPromptVisible = false;
		}


		void ResultUI::Update(const CelebrationTimeline& timeline)
		{
			// ---- GAME CLEAR（大きい所から弾むように縮む） ----
			const float logoRate = timeline.GetLogoRate();
			m_isLogoVisible = logoRate > 0.0f;
			const float logoScale = math::Lerp(LOGO_START_SCALE, 1.0f, EaseOutBack(logoRate));
			const float logoAlpha = math::Clamp01(logoRate * 3.0f);
			m_logo.Place(LOGO_X, LOGO_Y, logoScale, Vector4(1.0f, 1.0f, 1.0f, logoAlpha));

			// ---- CONGRATULATIONS!（ロゴより少し遅れて、下からふわっと） ----
			const float congratulationsRate = math::SmoothStep((logoRate - CONGRATULATIONS_DELAY) / (1.0f - CONGRATULATIONS_DELAY));
			m_congratulations.Place(LOGO_X, CONGRATULATIONS_Y - 20.0f * (1.0f - congratulationsRate), 1.0f,
				Vector4(1.0f, 0.95f, 0.8f, congratulationsRate));

			// ---- A TITLE（タイトルへ戻れるようになったら、点滅させる） ----
			m_isPromptVisible = timeline.CanLeave();
			const float promptTime = timeline.GetPromptTime();
			const float fadeIn = math::Clamp01(promptTime / PROMPT_FADE_TIME);
			const float blink = PROMPT_MIN_ALPHA + (1.0f - PROMPT_MIN_ALPHA) * (0.5f + 0.5f * std::cos(promptTime * PROMPT_BLINK_SPEED * TWO_PI));
			const float promptAlpha = fadeIn * blink;
			m_buttonA.Place(BUTTON_A_X, PROMPT_Y, 1.0f, Vector4(1.0f, 1.0f, 1.0f, promptAlpha));
			m_labelTitle.Place(LABEL_TITLE_X, PROMPT_Y, 1.0f, Vector4(1.0f, 1.0f, 1.0f, promptAlpha));
		}


		void ResultUI::Render(RenderContext& rc)
		{
			if (m_isLogoVisible)
			{
				m_logo.Draw(rc);
				m_congratulations.Draw(rc);
			}
			if (m_isPromptVisible)
			{
				m_buttonA.Draw(rc);
				m_labelTitle.Draw(rc);
			}
		}
	}
}
