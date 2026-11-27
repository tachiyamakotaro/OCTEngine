/**
 * @file StageCarousel.cpp
 * @brief ステージセレクトの札を横に流れるように並べる計算
 */
#include "stdafx.h"
#include "StageCarousel.h"
#include "Source/Core/Math/MathUtil.h"
#include <cmath>


namespace app
{
	namespace ui
	{
		namespace
		{
			/** 真ん中の札の拡大率（選んでいる札。脈打つ分は StageSelectMenu が足す） */
			constexpr float CENTER_SCALE = 1.1f;
			/** 真ん中から札1枚離れるごとに小さくする量 */
			constexpr float SCALE_STEP = 0.2f;
			/** ここ（札の数）より外は、もう小さくしない */
			constexpr float SCALE_DISTANCE = 3.0f;
			/** となりの札どうしのすき間（札の大きさが変わっても、すき間はそろえる） */
			constexpr float CARD_GAP = 30.0f;
			/** 薄くなり始める距離と、消える距離（札の数） */
			constexpr float FADE_START = 2.0f;
			constexpr float FADE_END = 3.4f;
			/**
			 * 流れる速さ（1秒に、残りの距離のどれだけ近づくか。大きいほど速く止まる）
			 * 14 で、1枚分を約 0.25 秒でほぼ止まる
			 */
			constexpr float FOLLOW_SPEED = 14.0f;
			/** 残りがこれより近ければ、ぴったり止める */
			constexpr float SNAP_DISTANCE = 0.001f;
		}


		StageCarousel::StageCarousel(const Vector2& center, const int stageCount)
			: m_center(center)
			// 0 個でも割り算で壊れないように、1 個はあることにする
			, m_stageCount(static_cast<float>(stageCount < 1 ? 1 : stageCount))
			, m_scroll(0.0f)
		{}


		void StageCarousel::SnapTo(const int stageIndex)
		{
			m_scroll = static_cast<float>(stageIndex);
		}


		void StageCarousel::Update(const int stageIndex)
		{
			// 残りの距離の一定の割合ずつ近づく（初めは速く、止まる前はゆっくり）
			// 輪の近いほうの向きへ流れる（FINAL → STAGE 1 は右へ1枚分。巻き戻らない）
			const float target = static_cast<float>(stageIndex);
			const float remain = WrapDifference(target - m_scroll);
			if (math::Abs(remain) < SNAP_DISTANCE)
			{
				m_scroll = target;
				return;
			}
			const float deltaTime = g_gameTime->GetFrameDeltaTime();
			const float rate = 1.0f - std::exp(-FOLLOW_SPEED * deltaTime);
			m_scroll += remain * rate;
			// 輪の上（0 〜 ステージの数）に戻しておく
			m_scroll -= m_stageCount * std::floor(m_scroll / m_stageCount);
		}


		StageCardPose StageCarousel::GetPose(const int stageIndex) const
		{
			const float signedDistance = GetDistance(stageIndex);
			const float distance = math::Abs(signedDistance);
			StageCardPose pose;
			pose.center = Vector2(m_center.x + math::Sign(signedDistance) * GetOffsetX(distance), m_center.y);
			pose.scale = GetScale(distance);
			pose.alpha = GetAlpha(distance);
			return pose;
		}


		bool StageCarousel::IsVisible(const int stageIndex) const
		{
			return GetAlpha(math::Abs(GetDistance(stageIndex))) > 0.0f;
		}


		float StageCarousel::GetDistance(const int stageIndex) const
		{
			return WrapDifference(static_cast<float>(stageIndex) - m_scroll);
		}


		float StageCarousel::WrapDifference(const float difference) const
		{
			// 例：10 ステージで、真ん中が FINAL（9）なら、STAGE 1（0）は 0 − 9 = −9 → +1（右どなり）
			return difference - m_stageCount * std::floor(difference / m_stageCount + 0.5f);
		}


		float StageCarousel::GetScale(const float distance)
		{
			return CENTER_SCALE - SCALE_STEP * math::Min(distance, SCALE_DISTANCE);
		}


		float StageCarousel::GetOffsetX(const float distance)
		{
			// となりどうしの真ん中の間 = 両方の札の幅の半分 + すき間。拡大率が距離に比例して小さくなるので、足し合わせると d の2次式になる
			//   x(d) = d × (W × CENTER_SCALE + CARD_GAP) − (W × SCALE_STEP ÷ 2) × d²   （d ≦ SCALE_DISTANCE）
			// SCALE_DISTANCE より外は、一番小さい札の間で並べる
			constexpr float W = StageCard::WIDTH;
			const float d = math::Min(distance, SCALE_DISTANCE);
			const float inner = d * (W * CENTER_SCALE + CARD_GAP) - (W * SCALE_STEP * 0.5f) * d * d;
			const float outer = math::Max(distance - SCALE_DISTANCE, 0.0f) * (W * GetScale(SCALE_DISTANCE) + CARD_GAP);
			return inner + outer;
		}


		float StageCarousel::GetAlpha(const float distance)
		{
			return 1.0f - math::SmoothStep((distance - FADE_START) / (FADE_END - FADE_START));
		}
	}
}
