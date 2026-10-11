/**
 * @file Vec2.h
 * @brief 横スクロール平面(XY)用の2次元ベクトル
 * @details ゲームロジック（移動・当たり判定・ステート）は XY 平面だけで完結させ、
 *          描画するときにだけエンジンの Vector3 に変換する（CharacterBase 参照）。
 *          エンジンの nsK2EngineLow::Vector2 と名前が衝突しないよう Vec2 という名前にしている。
 */
#pragma once
#include <cmath>
#include "MathUtil.h"


namespace app
{
	namespace math
	{
		/**
		 * @brief 2次元ベクトル（X:右が正 / Y:上が正）
		 */
		struct Vec2
		{
			float x = 0.0f;
			float y = 0.0f;


			constexpr Vec2() = default;
			constexpr Vec2(const float x_, const float y_) : x(x_), y(y_) {}


			constexpr Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
			constexpr Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
			constexpr Vec2 operator*(const float s) const { return { x * s, y * s }; }
			Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
			Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
			Vec2& operator*=(const float s) { x *= s; y *= s; return *this; }

			/** 長さ */
			float Length() const { return std::sqrt(x * x + y * y); }
			/** 長さの2乗 */
			float LengthSq() const { return x * x + y * y; }


			static constexpr Vec2 Zero() { return { 0.0f, 0.0f }; }
		};


		/** 内積 */
		inline float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
		/** 外積（2Dなのでスカラー。a から b が反時計回りなら正） */
		inline float Cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }
		/** 正規化（長さ0ならそのまま） */
		inline Vec2 Normalize(const Vec2& v)
		{
			const float len = v.Length();
			return len > 0.0f ? v * (1.0f / len) : v;
		}
		/** 角度（度）から単位ベクトルを作る（0度=右、90度=上） */
		inline Vec2 DirectionFromDeg(const float deg)
		{
			const float rad = DegToRad(deg);
			return { std::cos(rad), std::sin(rad) };
		}
		/** 法線 n の面で反射したベクトル */
		inline Vec2 Reflect(const Vec2& v, const Vec2& n)
		{
			return v - n * (2.0f * Dot(v, n));
		}

		/** ベクトルの線形補間 */
		inline Vec2 Lerp(const Vec2& a, const Vec2& b, const float t)
		{
			return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
		}
	}
}
