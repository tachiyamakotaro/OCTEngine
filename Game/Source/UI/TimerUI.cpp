/**
 * @file TimerUI.cpp
 * @brief 時間制限の表示
 */
#include "stdafx.h"
#include "TimerUI.h"
#include "imgui.h"


namespace
{
	// ---- 画像（パス） ----
	const char* const FRAME_PATH = "Assets/sprite/timer/frame.dds";
	const char* const ICON_PATH = "Assets/sprite/timer/icon_clock.dds";
	const char* const COLON_PATH = "Assets/sprite/timer/colon.dds";
	const char* const GAUGE_PATH = "Assets/sprite/timer/gauge.dds";
	const char* const TIME_UP_PATH = "Assets/sprite/timer/time_up.dds";
	const char* const SHADE_PATH = "Assets/sprite/fade.dds";
	/** 数字の画像（digit_0.dds 〜 digit_9.dds） */
	const char* const DIGIT_PATHS[10] = {
		"Assets/sprite/timer/digit_0.dds", "Assets/sprite/timer/digit_1.dds",
		"Assets/sprite/timer/digit_2.dds", "Assets/sprite/timer/digit_3.dds",
		"Assets/sprite/timer/digit_4.dds", "Assets/sprite/timer/digit_5.dds",
		"Assets/sprite/timer/digit_6.dds", "Assets/sprite/timer/digit_7.dds",
		"Assets/sprite/timer/digit_8.dds", "Assets/sprite/timer/digit_9.dds",
	};

	// ---- 画像の元の大きさ（ピクセル。縦横比を出すためだけに使う） ----
	constexpr float FRAME_PIXEL_W = 512.0f, FRAME_PIXEL_H = 160.0f;
	constexpr float DIGIT_PIXEL_W = 96.0f, DIGIT_PIXEL_H = 128.0f;
	constexpr float COLON_PIXEL_W = 48.0f, COLON_PIXEL_H = 128.0f;
	constexpr float ICON_PIXEL_SIZE = 128.0f;
	constexpr float TIME_UP_PIXEL_W = 1024.0f, TIME_UP_PIXEL_H = 256.0f;

	// ---- 配置（画面の真ん中が原点、上が +Y。画面は 1920x1080。上の端は Y=540） ----
	/** 背景の板 */
	constexpr float PANEL_X = 0.0f;
	constexpr float PANEL_Y = 465.0f;
	constexpr float PANEL_HEIGHT = 120.0f;
	/** 時計のアイコン（板の左側） */
	constexpr float ICON_X = -125.0f;
	constexpr float ICON_Y = 470.0f;
	constexpr float ICON_SIZE = 66.0f;
	/** 数字（並べたまとまりの真ん中の位置） */
	constexpr float DIGITS_CENTER_X = 35.0f;
	constexpr float DIGITS_Y = 478.0f;
	constexpr float DIGIT_HEIGHT = 80.0f;
	/** 数字1文字ぶんの送り幅（画像の幅より狭くして、数字どうしを詰める） */
	constexpr float DIGIT_STEP = 50.0f;
	/** 「:」の送り幅 */
	constexpr float COLON_STEP = 30.0f;
	/** ゲージ（数字の下。左端の位置と長さ） */
	constexpr float GAUGE_LEFT_X = -80.0f;
	constexpr float GAUGE_Y = 423.0f;
	constexpr float GAUGE_WIDTH = 230.0f;
	constexpr float GAUGE_HEIGHT = 10.0f;
	/** TIME UP の文字 */
	constexpr float TIME_UP_Y = 80.0f;
	constexpr float TIME_UP_HEIGHT = 190.0f;

	// ---- 演出 ----
	/** 数字が跳ねる時間（秒） */
	constexpr float PULSE_TIME = 0.3f;
	/** 跳ねた瞬間の大きさ（1 + この値） */
	constexpr float PULSE_SCALE = 0.25f;
	/** TIME UP が飛び出す時間（秒） */
	constexpr float TIME_UP_POP_TIME = 0.25f;
	/** TIME UP の飛び出し始めの大きさ（1 + この値から 1 に縮む） */
	constexpr float TIME_UP_POP_SCALE = 0.8f;
	/** 時間切れのときに画面を暗くする時間と濃さ */
	constexpr float TIME_UP_SHADE_TIME = 0.3f;
	constexpr float TIME_UP_SHADE_ALPHA = 0.4f;
}


namespace app
{
	namespace ui
	{
		namespace
		{
			/**
			 * @brief 時間制限の表示の色
			 * @details COLOR_ で始まる名前は Windows のヘッダーのマクロとぶつかることがあるので、名前空間に入れている
			 */
			namespace timer_color
			{
				/** ふつう（白） */
				const Vector4 Normal(1.0f, 1.0f, 1.0f, 1.0f);
				/** 注意（オレンジ） */
				const Vector4 Caution(1.0f, 0.7f, 0.15f, 1.0f);
				/** 危険・時間切れ（赤） */
				const Vector4 Danger(1.0f, 0.22f, 0.18f, 1.0f);
				/** 背景の板（暗い半透明） */
				const Vector4 Frame(0.05f, 0.06f, 0.09f, 0.55f);
				/** 危険のときの背景の板（暗い赤） */
				const Vector4 FrameDanger(0.4f, 0.03f, 0.03f, 0.65f);
				/** ゲージの下地 */
				const Vector4 GaugeBack(0.0f, 0.0f, 0.0f, 0.5f);
				/** ふつうのときのゲージ（少し青い白） */
				const Vector4 GaugeNormal(0.75f, 0.9f, 1.0f, 1.0f);
			}


			/** a から b へ t（0〜1）の割合で近づけた色 */
			Vector4 LerpColor(const Vector4& a, const Vector4& b, const float t)
			{
				return Vector4(
					a.x + (b.x - a.x) * t,
					a.y + (b.y - a.y) * t,
					a.z + (b.z - a.z) * t,
					a.w + (b.w - a.w) * t);
			}


			/** 0〜1 に収める */
			float Clamp01(const float value)
			{
				return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
			}
		}


		TimerUI::TimerUI()
			: m_isVisible(true)
			, m_isEnabled(false)
			, m_isTimeUp(false)
			, m_prevLevel(timer::TimeLevel::Normal)
			, m_pulseTimer(0.0f)
			, m_timeUpTimer(0.0f)
			, m_gaugeRate(1.0f)
			, m_usedSlotCount(3)
			, m_slotDigits{ 0, 0, 0, 0 }
		{}


		TimerUI::~TimerUI()
		{}


		void TimerUI::Init()
		{
			m_frame.Init(FRAME_PATH, FRAME_PIXEL_W, FRAME_PIXEL_H, PANEL_HEIGHT);
			m_icon.Init(ICON_PATH, ICON_PIXEL_SIZE, ICON_PIXEL_SIZE, ICON_SIZE);
			for (auto& slot : m_digits)
			{
				for (int digit = 0; digit < 10; ++digit)
				{
					slot[digit].Init(DIGIT_PATHS[digit], DIGIT_PIXEL_W, DIGIT_PIXEL_H, DIGIT_HEIGHT);
				}
			}
			m_colon.Init(COLON_PATH, COLON_PIXEL_W, COLON_PIXEL_H, DIGIT_HEIGHT);

			// ゲージは横だけ縮めるので、UISprite（縦横同じ拡大率）ではなく SpriteRender をそのまま使う
			m_gaugeBack.Init(GAUGE_PATH, GAUGE_WIDTH, GAUGE_HEIGHT);
			m_gaugeFill.Init(GAUGE_PATH, GAUGE_WIDTH, GAUGE_HEIGHT);
			m_gaugeBack.SetPosition(Vector3(GAUGE_LEFT_X + GAUGE_WIDTH * 0.5f, GAUGE_Y, 0.0f));
			m_gaugeBack.SetMulColor(timer_color::GaugeBack);
			m_gaugeBack.Update();

			m_timeUpShade.InitFullScreen(SHADE_PATH);
			m_timeUp.Init(TIME_UP_PATH, TIME_UP_PIXEL_W, TIME_UP_PIXEL_H, TIME_UP_HEIGHT);
		}


		void TimerUI::Update(const timer::StageTimer& stageTimer, const float deltaTime)
		{
			m_isEnabled = stageTimer.IsEnabled();
			if (!m_isEnabled)
			{
				return;
			}

			// ---- 数字が跳ねる合図：危険の間は表示の秒数が変わるたび、注意になった瞬間に1回 ----
			const timer::TimeLevel level = stageTimer.GetLevel();
			if (level == timer::TimeLevel::Danger && stageTimer.IsSecondChangedThisFrame())
			{
				m_pulseTimer = PULSE_TIME;
			}
			else if (level == timer::TimeLevel::Caution && m_prevLevel == timer::TimeLevel::Normal)
			{
				m_pulseTimer = PULSE_TIME;
			}
			m_prevLevel = level;
			m_pulseTimer = m_pulseTimer > deltaTime ? m_pulseTimer - deltaTime : 0.0f;
			// 跳ねた瞬間がいちばん大きく、なめらかに戻る
			const float pulse = m_pulseTimer / PULSE_TIME;
			const float pulseScale = 1.0f + PULSE_SCALE * pulse * pulse;

			// ---- 時間切れの演出の時間 ----
			m_isTimeUp = stageTimer.IsTimeUp();
			m_timeUpTimer = m_isTimeUp ? m_timeUpTimer + deltaTime : 0.0f;

			// ---- 色（段階で変える） ----
			Vector4 textColor = timer_color::Normal;
			Vector4 gaugeColor = timer_color::GaugeNormal;
			Vector4 frameColor = timer_color::Frame;
			switch (level)
			{
			case timer::TimeLevel::Caution:
				textColor = timer_color::Caution;
				gaugeColor = timer_color::Caution;
				break;
			case timer::TimeLevel::Danger:
				textColor = timer_color::Danger;
				gaugeColor = timer_color::Danger;
				// 跳ねるのに合わせて板も赤く光る
				frameColor = LerpColor(timer_color::Frame, timer_color::FrameDanger, 0.5f + 0.5f * pulse);
				break;
			case timer::TimeLevel::TimeUp:
				textColor = timer_color::Danger;
				gaugeColor = timer_color::Danger;
				frameColor = timer_color::FrameDanger;
				break;
			case timer::TimeLevel::Normal:
			default:
				break;
			}

			// ---- 置く ----
			m_frame.Place(PANEL_X, PANEL_Y, 1.0f, frameColor);
			m_icon.Place(ICON_X, ICON_Y, 1.0f, textColor);
			PlaceDigits(stageTimer.GetDisplaySeconds(), textColor, pulseScale);
			PlaceGauge(stageTimer.GetRemainingRate(), gaugeColor);
			if (m_isTimeUp)
			{
				PlaceTimeUp();
			}
		}


		void TimerUI::PlaceDigits(const int displaySeconds, const Vector4& color, const float scale)
		{
			// 表示できるのは 99:59 まで
			constexpr int MAX_DISPLAY_SECONDS = 99 * 60 + 59;
			int seconds = displaySeconds < 0 ? 0 : displaySeconds;
			if (seconds > MAX_DISPLAY_SECONDS)
			{
				seconds = MAX_DISPLAY_SECONDS;
			}
			const int minutes = seconds / 60;
			const int secondsInMinute = seconds % 60;

			// 場所ごとの数字（0:分の十の位 1:分の一の位 2:秒の十の位 3:秒の一の位）
			m_slotDigits[0] = minutes / 10;
			m_slotDigits[1] = minutes % 10;
			m_slotDigits[2] = secondsInMinute / 10;
			m_slotDigits[3] = secondsInMinute % 10;
			// 10分未満なら分の十の位は出さない（M:SS）
			m_usedSlotCount = minutes >= 10 ? 4 : 3;
			const int firstSlot = SLOT_COUNT - m_usedSlotCount;

			// 左から並べたときの全体の幅（数字の数 × 送り幅 + 「:」）
			const float totalWidth = DIGIT_STEP * static_cast<float>(m_usedSlotCount) + COLON_STEP;
			float x = DIGITS_CENTER_X - totalWidth * 0.5f;

			// 跳ねるときは、まとまりの真ん中を中心に全体を大きくする（文字の間も広げる）
			auto placeOne = [&](UISprite& sprite, const float step)
				{
					const float centerX = x + step * 0.5f;
					const float scaledX = DIGITS_CENTER_X + (centerX - DIGITS_CENTER_X) * scale;
					sprite.Place(scaledX, DIGITS_Y, scale, color);
					x += step;
				};
			for (int slot = firstSlot; slot < SLOT_COUNT; ++slot)
			{
				// 秒の十の位の前に「:」を置く
				if (slot == 2)
				{
					placeOne(m_colon, COLON_STEP);
				}
				placeOne(m_digits[slot][m_slotDigits[slot]], DIGIT_STEP);
			}
		}


		void TimerUI::PlaceGauge(const float rate, const Vector4& color)
		{
			// 左端をそろえて、右から縮める（大きさ 1 の画像の真ん中が、縮めた棒の真ん中に来るように動かす）
			const float clamped = Clamp01(rate);
			m_gaugeRate = clamped;
			m_gaugeFill.SetPosition(Vector3(GAUGE_LEFT_X + GAUGE_WIDTH * clamped * 0.5f, GAUGE_Y, 0.0f));
			m_gaugeFill.SetScale(Vector3(clamped, 1.0f, 1.0f));
			m_gaugeFill.SetMulColor(color);
			m_gaugeFill.Update();
		}


		void TimerUI::PlaceTimeUp()
		{
			// 画面を少しずつ暗くする
			const float shade = Clamp01(m_timeUpTimer / TIME_UP_SHADE_TIME) * TIME_UP_SHADE_ALPHA;
			m_timeUpShade.Place(0.0f, 0.0f, 1.0f, Vector4(1.0f, 1.0f, 1.0f, shade));

			// 大きい状態から縮んで止まる（同時にだんだん見えてくる）
			const float pop = Clamp01(m_timeUpTimer / TIME_UP_POP_TIME);
			const float rest = 1.0f - pop;
			const float scale = 1.0f + TIME_UP_POP_SCALE * rest * rest;
			Vector4 color = timer_color::Danger;
			color.w = pop;
			m_timeUp.Place(0.0f, TIME_UP_Y, scale, color);
		}


		void TimerUI::DrawSettingsUI()
		{
			ImGui::PushID("TimerUI");
			ImGui::Checkbox("Show Timer UI", &m_isVisible);
			ImGui::PopID();
		}


		void TimerUI::Render(RenderContext& rc)
		{
			if (!m_isVisible || !m_isEnabled)
			{
				return;
			}
			// 描いた順に重なる
			// 時間切れ：まず画面を暗くする（タイマーは暗くしない）
			if (m_isTimeUp)
			{
				m_timeUpShade.Draw(rc);
			}
			m_frame.Draw(rc);
			m_gaugeBack.Draw(rc);
			if (m_gaugeRate > 0.0f)
			{
				m_gaugeFill.Draw(rc);
			}
			m_icon.Draw(rc);
			const int firstSlot = SLOT_COUNT - m_usedSlotCount;
			for (int slot = firstSlot; slot < SLOT_COUNT; ++slot)
			{
				m_digits[slot][m_slotDigits[slot]].Draw(rc);
			}
			m_colon.Draw(rc);

			// 時間切れ：いちばん上に TIME UP
			if (m_isTimeUp)
			{
				m_timeUp.Draw(rc);
			}
		}
	}
}
