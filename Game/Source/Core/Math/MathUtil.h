/**
 * @file MathUtil.h
 * @brief 数値計算の小さなユーティリティ
 * @note windows.h の min/max マクロと衝突しないよう、std::min/max は使わずに自前で用意している
 */
#pragma once


namespace app
{
	namespace math
	{
		/** 円周率 */
		constexpr float PI = 3.14159265358979f;
		/** 度 → ラジアン */
		inline float DegToRad(const float deg) { return deg * (PI / 180.0f); }
		/** ラジアン → 度 */
		inline float RadToDeg(const float rad) { return rad * (180.0f / PI); }

		/** 絶対値 */
		inline float Abs(const float v) { return v < 0.0f ? -v : v; }
		/** 小さい方 */
		inline float Min(const float a, const float b) { return a < b ? a : b; }
		/** 大きい方 */
		inline float Max(const float a, const float b) { return a > b ? a : b; }
		/** 範囲内に収める */
		inline float Clamp(const float v, const float lo, const float hi) { return v < lo ? lo : (v > hi ? hi : v); }
		/** 0〜1に収める */
		inline float Clamp01(const float v) { return Clamp(v, 0.0f, 1.0f); }
		/** 符号（0は0） */
		inline float Sign(const float v) { return v > 0.0f ? 1.0f : (v < 0.0f ? -1.0f : 0.0f); }
		/** 線形補間 */
		inline float Lerp(const float a, const float b, const float t) { return a + (b - a) * t; }
		/** なめらかな補間係数（0〜1 → 0〜1） */
		inline float SmoothStep(float t)
		{
			t = Clamp01(t);
			return t * t * (3.0f - 2.0f * t);
		}
		/**
		 * @brief 角度（度）を 0〜360 に収める
		 */
		inline float NormalizeDeg(float deg)
		{
			while (deg >= 360.0f) { deg -= 360.0f; }
			while (deg < 0.0f) { deg += 360.0f; }
			return deg;
		}
		/**
		 * @brief current を target に向けて最大 maxDelta だけ近づける
		 * @details 加速・減速の計算に使う（行き過ぎない）
		 */
		inline float MoveTowards(const float current, const float target, const float maxDelta)
		{
			if (Abs(target - current) <= maxDelta)
			{
				return target;
			}
			return current + (target > current ? maxDelta : -maxDelta);
		}
	}
}
