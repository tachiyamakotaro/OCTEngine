/**
 * @file MirrorPromptUI.cpp
 * @brief 反射鏡の近くで出すボタンのインタラクト表示（B で回転 / RB で移動）
 */
#include "stdafx.h"
#include "MirrorPromptUI.h"
#include "imgui.h"
#include "Source/Actor/Character/CharacterBase.h"
#include "Source/Stage/Gimmick/LightMirror.h"
#include "Source/Stage/MirrorInteractor.h"
#include "Source/Stage/Stage.h"
#include <cmath>


namespace
{
	// ---- 画像（パス） ----
	const char* const PLATE_PATH = "Assets/sprite/prompt/plate.dds";
	const char* const POINTER_PATH = "Assets/sprite/prompt/pointer.dds";
	const char* const BUTTON_B_PATH = "Assets/sprite/prompt/button_b.dds";
	const char* const BUTTON_RB_PATH = "Assets/sprite/prompt/button_rb.dds";
	const char* const ICON_ROTATE_CW_PATH = "Assets/sprite/prompt/icon_rotate_cw.dds";
	const char* const ICON_ROTATE_CCW_PATH = "Assets/sprite/prompt/icon_rotate_ccw.dds";
	const char* const ICON_MOVE_PATH = "Assets/sprite/prompt/icon_move.dds";

	// ---- 画像の元の大きさ（ピクセル。縦横比を出すためだけに使う） ----
	constexpr float PLATE_PIXEL_W = 256.0f, PLATE_PIXEL_H = 128.0f;
	constexpr float POINTER_PIXEL_W = 64.0f, POINTER_PIXEL_H = 40.0f;
	constexpr float BUTTON_B_PIXEL_SIZE = 128.0f;
	constexpr float BUTTON_RB_PIXEL_W = 192.0f, BUTTON_RB_PIXEL_H = 112.0f;
	constexpr float ICON_PIXEL_SIZE = 128.0f;

	// ---- 大きさと並べ方（画面のピクセル。画面は 1920x1080） ----
	/** 板の高さ（幅は縦横比から決まる） */
	constexpr float PLATE_HEIGHT = 72.0f;
	/** 板の幅 */
	constexpr float PLATE_WIDTH = PLATE_HEIGHT * PLATE_PIXEL_W / PLATE_PIXEL_H;
	/** 板と板のすき間（両方出すとき） */
	constexpr float PLATE_GAP = 10.0f;
	/** 鏡を指す三角の高さ */
	constexpr float POINTER_HEIGHT = 20.0f;
	/** 三角を板の下にどれだけ重ねるか（すき間が見えないように） */
	constexpr float POINTER_OVERLAP = 2.0f;
	/** B ボタンの高さ */
	constexpr float BUTTON_B_HEIGHT = 54.0f;
	/** RB ボタンの高さ */
	constexpr float BUTTON_RB_HEIGHT = 42.0f;
	/** 板の真ん中から見た、ボタンの横の位置 */
	constexpr float BUTTON_OFFSET_X = -32.0f;
	/** アイコンの大きさ */
	constexpr float ICON_SIZE = 48.0f;
	/** 板の真ん中から見た、アイコンの横の位置 */
	constexpr float ICON_OFFSET_X = 38.0f;
	/** 画面の端からこれ以上は寄せない */
	constexpr float SCREEN_MARGIN = 16.0f;

	// ---- 演出 ----
	/** 出てくるまでの時間（秒） */
	constexpr float APPEAR_TIME = 0.15f;
	/** 消えるまでの時間（秒） */
	constexpr float FADE_OUT_TIME = 0.12f;
	/** ポップの時間（秒） */
	constexpr float POP_TIME = 0.25f;
	/** ポップの出始めの大きさ（ここから少し大きくなりすぎてから 1 に落ち着く） */
	constexpr float POP_START_SCALE = 0.6f;
	/** ふわふわ上下する幅（ピクセル）と速さ */
	constexpr float BOB_AMPLITUDE = 4.0f;
	constexpr float BOB_SPEED = 3.0f;
	/** B を押したときの演出の時間（秒） */
	constexpr float ROTATE_PULSE_TIME = 0.3f;
	/** B を押した瞬間の大きさ（1 + この値） */
	constexpr float ROTATE_PULSE_SCALE = 0.3f;
	/** 引きずっている間の RB の大きさ（押している見た目） */
	constexpr float DRAG_PRESSED_SCALE = 0.9f;
	/** 引きずっている間、左右の矢印が揺れる幅（ピクセル）と速さ */
	constexpr float DRAG_WOBBLE_AMPLITUDE = 5.0f;
	constexpr float DRAG_WOBBLE_SPEED = 12.0f;
	/** 板の濃さ */
	constexpr float PLATE_ALPHA = 0.62f;
	/** 表示全体の大きさ（1 で元の大きさ。三角の先を中心に縮める） */
	constexpr float UI_SCALE = 0.75f;
	/** B を押したときに、回転のアイコンを1回転させるか（今は切っている。B が跳ねる演出は残す） */
	constexpr bool IS_ROTATE_SPIN_ENABLED = false;

	/** 鏡のいちばん上から表示までの高さの初期値（ワールドの長さ） */
	constexpr float DEFAULT_ANCHOR_OFFSET_Y = 20.0f;
}


namespace app
{
	namespace ui
	{
		namespace
		{
			/**
			 * @brief インタラクト表示の色
			 * @details COLOR_ で始まる名前は Windows のヘッダーのマクロとぶつかることがあるので、名前空間に入れている
			 */
			namespace prompt_color
			{
				/** 背景の板と三角（暗い色。濃さは PLATE_ALPHA） */
				const Vector4 Plate(0.05f, 0.06f, 0.09f, 1.0f);
			}


			/** 0〜1 に収める */
			float Clamp01(const float value)
			{
				return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
			}


			/** min〜max に収める（min > max のときは真ん中） */
			float ClampRange(const float value, const float min, const float max)
			{
				if (min > max)
				{
					return (min + max) * 0.5f;
				}
				return value < min ? min : (value > max ? max : value);
			}


			/** 少し行きすぎてから止まる動き（0〜1 → 0〜1） */
			float EaseOutBack(const float t)
			{
				constexpr float c1 = 1.70158f;
				constexpr float c3 = c1 + 1.0f;
				const float u = t - 1.0f;
				return 1.0f + c3 * u * u * u + c1 * u * u;
			}


			/** 色の透明度だけ掛ける */
			Vector4 WithAlpha(const Vector4& color, const float alpha)
			{
				return Vector4(color.x, color.y, color.z, color.w * alpha);
			}
		}


		MirrorPromptUI::MirrorPromptUI()
			: m_isVisible(true)
			, m_anchorOffsetY(DEFAULT_ANCHOR_OFFSET_Y)
			, m_prevIndex(-1)
			, m_entries{ EntryKind::Rotate, EntryKind::Move }
			, m_entryCount(0)
			, m_anchorX(0.0f)
			, m_anchorY(0.0f)
			, m_isClockwise(true)
			, m_isDragging(false)
			, m_alpha(0.0f)
			, m_popTimer(0.0f)
			, m_rotatePulseTimer(0.0f)
			, m_elapsed(0.0f)
		{}


		MirrorPromptUI::~MirrorPromptUI()
		{}


		void MirrorPromptUI::Init()
		{
			for (UISprite& plate : m_plates)
			{
				plate.Init(PLATE_PATH, PLATE_PIXEL_W, PLATE_PIXEL_H, PLATE_HEIGHT);
			}
			m_pointer.Init(POINTER_PATH, POINTER_PIXEL_W, POINTER_PIXEL_H, POINTER_HEIGHT);
			m_buttonB.Init(BUTTON_B_PATH, BUTTON_B_PIXEL_SIZE, BUTTON_B_PIXEL_SIZE, BUTTON_B_HEIGHT);
			m_buttonRB.Init(BUTTON_RB_PATH, BUTTON_RB_PIXEL_W, BUTTON_RB_PIXEL_H, BUTTON_RB_HEIGHT);
			m_iconRotateCW.Init(ICON_ROTATE_CW_PATH, ICON_PIXEL_SIZE, ICON_PIXEL_SIZE, ICON_SIZE);
			m_iconRotateCCW.Init(ICON_ROTATE_CCW_PATH, ICON_PIXEL_SIZE, ICON_PIXEL_SIZE, ICON_SIZE);
			m_iconMove.Init(ICON_MOVE_PATH, ICON_PIXEL_SIZE, ICON_PIXEL_SIZE, ICON_SIZE);
		}


		void MirrorPromptUI::Update(const stage::Stage& targetStage, const stage::MirrorInteractor& interactor,
			const actor::CharacterBase& character, const bool canGrab, const bool isAllowed, const float deltaTime)
		{
			m_elapsed += deltaTime;
			m_rotatePulseTimer = m_rotatePulseTimer > deltaTime ? m_rotatePulseTimer - deltaTime : 0.0f;

			// ---- 操作できる鏡と、できること ----
			const int index = interactor.GetFocusedMirrorIndex();
			const stage::LightMirror* mirror = nullptr;
			if (index >= 0)
			{
				mirror = dynamic_cast<const stage::LightMirror*>(targetStage.GetGimmicks().Get(static_cast<size_t>(index)));
			}

			std::array<EntryKind, MAX_ENTRY_COUNT> entries = m_entries;
			int count = 0;
			if (mirror != nullptr && isAllowed)
			{
				if (mirror->isRotatable)
				{
					entries[count++] = EntryKind::Rotate;
				}
				// 日よけ板が近くて RB が板のほうに使われるときは出さない（引きずっている最中は出したまま）
				if (mirror->isMovable && (canGrab || interactor.IsDragging()))
				{
					entries[count++] = EntryKind::Move;
				}
			}

			if (m_isVisible && count > 0)
			{
				// 鏡がどの向きでも同じ高さに出るよう、傾きに関係なく「鏡の長さの半分」だけ上を使う
				const float topY = mirror->center.y + mirror->length * 0.5f + m_anchorOffsetY;
				float screenX = 0.0f;
				float screenY = 0.0f;
				if (WorldToScreen(mirror->center.x, topY, screenX, screenY))
				{
					m_anchorX = screenX;
					m_anchorY = screenY;
				}

				// 出てきた瞬間・別の鏡に変わった瞬間はポップし直す
				if (m_alpha <= 0.0f || index != m_prevIndex)
				{
					m_popTimer = 0.0f;
				}
				m_prevIndex = index;
				m_entries = entries;
				m_entryCount = count;
				m_alpha = Clamp01(m_alpha + deltaTime / APPEAR_TIME);
				m_popTimer += deltaTime;
				// 押したときに回る向き（MirrorInteractor：右を向いていれば時計回り）
				m_isClockwise = character.GetFacing() > 0;
				m_isDragging = interactor.IsDragging();
				if (interactor.IsRotatedThisFrame())
				{
					m_rotatePulseTimer = ROTATE_PULSE_TIME;
				}
			}
			else
			{
				// 消えていく間は、最後の位置と項目のまま薄くする
				m_prevIndex = -1;
				m_isDragging = false;
				m_alpha = Clamp01(m_alpha - deltaTime / FADE_OUT_TIME);
				if (m_alpha <= 0.0f)
				{
					m_entryCount = 0;
				}
			}

			PlaceAll();
		}


		bool MirrorPromptUI::WorldToScreen(const float worldX, const float worldY, float& outX, float& outY) const
		{
			// 鏡は奥行き 0 に置いている
			Vector4 clip(worldX, worldY, 0.0f, 1.0f);
			g_camera3D->GetViewProjectionMatrix().Apply(clip);
			if (clip.w <= 0.0001f)
			{
				// カメラの後ろ
				return false;
			}
			// -1〜1 の範囲 → 画面の真ん中が原点のピクセル（上が +Y）
			outX = clip.x / clip.w * static_cast<float>(FRAME_BUFFER_W) * 0.5f;
			outY = clip.y / clip.w * static_cast<float>(FRAME_BUFFER_H) * 0.5f;
			return true;
		}


		void MirrorPromptUI::PlaceAll()
		{
			if (m_entryCount <= 0 || m_alpha <= 0.0f)
			{
				return;
			}

			// ---- 全体の大きさ（ポップ）と、ふわふわ ----
			const float pop = Clamp01(m_popTimer / POP_TIME);
			const float groupScale = (POP_START_SCALE + (1.0f - POP_START_SCALE) * EaseOutBack(pop)) * UI_SCALE;
			const float bob = std::sin(m_elapsed * BOB_SPEED) * BOB_AMPLITUDE;

			// ---- 位置（三角の先が鏡の上。画面からはみ出さないように寄せる） ----
			const float halfScreenW = static_cast<float>(FRAME_BUFFER_W) * 0.5f;
			const float halfScreenH = static_cast<float>(FRAME_BUFFER_H) * 0.5f;
			const float totalWidth = PLATE_WIDTH * static_cast<float>(m_entryCount) + PLATE_GAP * static_cast<float>(m_entryCount - 1);
			const float totalHeight = POINTER_HEIGHT - POINTER_OVERLAP + PLATE_HEIGHT;
			// 三角の先（ここを中心に大きさを変える）
			const float tipY = ClampRange(m_anchorY + bob, -halfScreenH + SCREEN_MARGIN, halfScreenH - SCREEN_MARGIN - totalHeight);
			const float groupCenterX = ClampRange(m_anchorX,
				-halfScreenW + SCREEN_MARGIN + totalWidth * 0.5f, halfScreenW - SCREEN_MARGIN - totalWidth * 0.5f);
			// 三角は板からはみ出さない範囲で、なるべく鏡の真上を指す
			const float tipX = ClampRange(m_anchorX,
				groupCenterX - totalWidth * 0.5f + PLATE_HEIGHT * 0.5f, groupCenterX + totalWidth * 0.5f - PLATE_HEIGHT * 0.5f);

			// 三角の先から見た位置 (relX, relY) に置く（全体の大きさも掛ける）
			auto place = [&](UISprite& sprite, const float absX, const float relY, const float scale, const Vector4& color, const float rotationDeg = 0.0f)
				{
					const float x = tipX + (absX - tipX) * groupScale;
					const float y = tipY + relY * groupScale;
					sprite.Place(x, y, groupScale * scale, color, rotationDeg);
				};

			const Vector4 plateColor = WithAlpha(prompt_color::Plate, PLATE_ALPHA * m_alpha);
			const Vector4 iconColor = WithAlpha(menu_color::White, m_alpha);
			const float plateCenterY = POINTER_HEIGHT - POINTER_OVERLAP + PLATE_HEIGHT * 0.5f;

			// 三角（画像の真ん中が三角の高さの真ん中）
			place(m_pointer, tipX, POINTER_HEIGHT * 0.5f, 1.0f, plateColor);

			float plateX = groupCenterX - totalWidth * 0.5f + PLATE_WIDTH * 0.5f;
			for (int i = 0; i < m_entryCount; ++i)
			{
				place(m_plates[i], plateX, plateCenterY, 1.0f, plateColor);
				const float buttonX = plateX + BUTTON_OFFSET_X;
				const float iconX = plateX + ICON_OFFSET_X;

				if (m_entries[i] == EntryKind::Rotate)
				{
					// B を押した瞬間に跳ねて、アイコンが回る向きに1回転する
					const float pulse = m_rotatePulseTimer / ROTATE_PULSE_TIME;
					const float buttonScale = 1.0f + ROTATE_PULSE_SCALE * pulse * pulse;
					const float spin = 1.0f - pulse;
					const float spinEased = 1.0f - (1.0f - spin) * (1.0f - spin) * (1.0f - spin);
					// Z 回転はプラスで反時計回り。時計回りはマイナス
					const float spinDeg = IS_ROTATE_SPIN_ENABLED ? (m_isClockwise ? -360.0f : 360.0f) * spinEased : 0.0f;
					place(m_buttonB, buttonX, plateCenterY, buttonScale, iconColor);
					UISprite& icon = m_isClockwise ? m_iconRotateCW : m_iconRotateCCW;
					place(icon, iconX, plateCenterY, 1.0f, iconColor, spinDeg);
				}
				else
				{
					// 引きずっている間は、RB が黄色く小さくなって（押している見た目）、矢印が左右に揺れる
					const float buttonScale = m_isDragging ? DRAG_PRESSED_SCALE : 1.0f;
					const Vector4 buttonColor = WithAlpha(m_isDragging ? menu_color::Selected : menu_color::White, m_alpha);
					const float wobble = m_isDragging ? std::sin(m_elapsed * DRAG_WOBBLE_SPEED) * DRAG_WOBBLE_AMPLITUDE : 0.0f;
					place(m_buttonRB, buttonX, plateCenterY, buttonScale, buttonColor);
					place(m_iconMove, iconX + wobble, plateCenterY, 1.0f, iconColor);
				}
				plateX += PLATE_WIDTH + PLATE_GAP;
			}
		}


		void MirrorPromptUI::DrawSettingsUI()
		{
			ImGui::PushID("MirrorPromptUI");
			ImGui::Checkbox("Show Mirror Prompt", &m_isVisible);
			ImGui::DragFloat("Prompt Height", &m_anchorOffsetY, 1.0f, -200.0f, 400.0f, "%.0f");
			ImGui::PopID();
		}


		void MirrorPromptUI::Render(RenderContext& rc)
		{
			if (!m_isVisible || m_entryCount <= 0 || m_alpha <= 0.0f)
			{
				return;
			}
			// 描いた順に重なる：板 → 三角 → ボタンとアイコン
			for (int i = 0; i < m_entryCount; ++i)
			{
				m_plates[i].Draw(rc);
			}
			m_pointer.Draw(rc);
			for (int i = 0; i < m_entryCount; ++i)
			{
				if (m_entries[i] == EntryKind::Rotate)
				{
					m_buttonB.Draw(rc);
					(m_isClockwise ? m_iconRotateCW : m_iconRotateCCW).Draw(rc);
				}
				else
				{
					m_buttonRB.Draw(rc);
					m_iconMove.Draw(rc);
				}
			}
		}
	}
}
