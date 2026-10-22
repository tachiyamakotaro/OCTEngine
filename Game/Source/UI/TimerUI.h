/**
 * @file TimerUI.h
 * @brief 時間制限の表示（画面の上の真ん中に「時計のアイコン M:SS」と残り時間のゲージ。時間切れで TIME UP）
 * @details 見た目は全部画像（UISprite）。画像は Assets/sprite/timer/。
 *          数字の画像は白（＋暗いふち）なので、色は乗算カラーで変える。
 *          ・ふつう … 白
 *          ・注意（残り CAUTION_TIME 秒以下）… オレンジ。注意になった瞬間に1回跳ねる
 *          ・危険（残り DANGER_TIME 秒以下）… 赤。1秒ごとに数字が跳ねて、背景の板も赤くなる
 *          ・時間切れ … 0:00 のまま赤。画面の真ん中に TIME UP が飛び出す
 *
 *          数字は「1文字ぶんの場所（スロット）×0〜9の画像」を全部読み込んでおき、今の数字の画像だけ描く
 *          （このエンジンの SpriteRender は画像の一部だけを切り出して描けないため）。
 *          10分以上の制限時間なら MM:SS、10分未満なら M:SS で出す。
 *
 *          StageTimer（残り時間を数える）は見るだけで、変えない。
 */
#pragma once
#include "Source/Timer/StageTimer.h"
#include "Source/UI/UISprite.h"
#include <array>


namespace app
{
	namespace ui
	{
		/**
		 * @brief 時間制限の表示
		 */
		class TimerUI
		{
		public:
			TimerUI();
			~TimerUI();
			TimerUI(const TimerUI&) = delete;
			TimerUI& operator=(const TimerUI&) = delete;

			/** 画像を読み込む（シーンの Start で1回） */
			void Init();
			/**
			 * @brief タイマーに合わせて見た目を更新する（毎フレーム。タイマーの Update のあとに呼ぶ）
			 * @param stageTimer 表示するタイマー
			 * @param deltaTime 演出（跳ねる・TIME UP が飛び出す）に使う時間
			 */
			void Update(const timer::StageTimer& stageTimer, float deltaTime);
			/** 描く（シーンの Render から。ほかの UI より下に出したいなら先に呼ぶ） */
			void Render(RenderContext& rc);

			/** 表示するか（デバッグで隠したいとき用。時間制限がないステージでは自動で隠れる） */
			void SetVisible(bool isVisible) { m_isVisible = isVisible; }
			bool IsVisible() const { return m_isVisible; }

			/** ImGui の調整項目（表示の ON / OFF）。見出しは呼ぶ側で作る */
			void DrawSettingsUI();


		private:
			/** 数字の場所の数（MM:SS の M, M, S, S） */
			static constexpr int SLOT_COUNT = 4;

			/** 数字と「:」を並べる（10分以上なら MM:SS、10分未満なら M:SS） */
			void PlaceDigits(int displaySeconds, const Vector4& color, float scale);
			/** ゲージを置く（左端をそろえて、残りの割合だけ右から縮める） */
			void PlaceGauge(float rate, const Vector4& color);
			/** TIME UP を置く（飛び出して止まる） */
			void PlaceTimeUp();


		private:
			/** 表示するか */
			bool m_isVisible;
			/** 時間制限があるステージか（ないなら何も描かない） */
			bool m_isEnabled;
			/** 時間切れか */
			bool m_isTimeUp;
			/** 前のフレームの段階（注意になった瞬間を見つける） */
			timer::TimeLevel m_prevLevel;
			/** 数字が跳ねる演出の残り時間 */
			float m_pulseTimer;
			/** 時間切れになってからの時間（TIME UP の演出） */
			float m_timeUpTimer;
			/** ゲージの長さ（残りの割合。0 なら描かない） */
			float m_gaugeRate;
			/** 使っている数字の場所の数（M:SS なら 3、MM:SS なら 4） */
			int m_usedSlotCount;
			/** 場所ごとに今描く数字（0〜9） */
			std::array<int, SLOT_COUNT> m_slotDigits;

			// ---- 画像 ----
			/** 背景の板（白い角丸を暗く半透明にして使う） */
			UISprite m_frame;
			/** 時計のアイコン */
			UISprite m_icon;
			/** 数字（場所ごとに 0〜9 の10枚） */
			std::array<std::array<UISprite, 10>, SLOT_COUNT> m_digits;
			/** 「:」 */
			UISprite m_colon;
			/** ゲージの下地 */
			SpriteRender m_gaugeBack;
			/** ゲージ（残りの割合だけ横に縮める） */
			SpriteRender m_gaugeFill;
			/** 時間切れのときに画面を少し暗くする（フェードの黒い画像を半透明にして使う） */
			UISprite m_timeUpShade;
			/** TIME UP の文字 */
			UISprite m_timeUp;
		};
	}
}
