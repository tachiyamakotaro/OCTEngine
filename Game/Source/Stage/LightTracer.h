/**
 * @file LightTracer.h
 * @brief 光の経路を計算する（レイを飛ばして鏡で反射させる）
 * @details 光を出すギミック（GetEmittedRay）からまっすぐレイを飛ばし、
 *          全ギミックの IntersectLight で一番近くに当たった物を探して、その反応で処理を分ける。
 *          ・Reflect     … 反射して続ける（反射鏡）
 *          ・Stop        … 止まる（ブロック・閉じた扉・鏡の裏）
 *          ・Absorb      … 止まって、そのギミックを点灯させる（受光器）
 *          ・PassThrough … 止まらずに通り抜け、そのギミックを点灯させる（光の足場）
 *          光の計算はギミックの種類を知らないので、光に反応する新しいギミックを足しても、ここは書き換えなくてよい。
 */
#pragma once
#include <algorithm>
#include <vector>
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		class GimmickBase;
		class GimmickManager;


		/**
		 * @brief 光の線分（描画用）
		 */
		struct LightSegment
		{
			/** 始点 */
			math::Vec2 start;
			/** 終点 */
			math::Vec2 end;
			/** 何回反射したあとの線分か（0=光源から直接） */
			int bounce = 0;
		};


		/**
		 * @brief 光の計算結果
		 */
		struct LightTraceResult
		{
			/** 光の線分 */
			std::vector<LightSegment> segments;
			/** 光が当たって点灯したギミック（受光器・光の足場など） */
			std::vector<const GimmickBase*> litGimmicks;


			/** そのギミックに光が当たったか */
			bool IsLit(const GimmickBase& gimmick) const
			{
				return std::find(litGimmicks.begin(), litGimmicks.end(), &gimmick) != litGimmicks.end();
			}
			void Clear()
			{
				segments.clear();
				litGimmicks.clear();
			}
		};


		/**
		 * @brief 光の経路の計算
		 */
		class LightTracer
		{
		public:
			/** 反射の最大回数（鏡の間で無限に反射し続けるのを防ぐ） */
			int maxBounces = 32;
			/** 光が届く最大距離（1本の線分あたり） */
			float maxDistance = 10000.0f;


		public:
			/**
			 * @brief 全ギミックの光について経路を計算する
			 */
			void Trace(const GimmickManager& gimmicks, LightTraceResult& out) const;


		private:
			/**
			 * @brief 1本の光を追いかける
			 */
			void TraceBeam(const GimmickManager& gimmicks, math::Vec2 origin, math::Vec2 direction, LightTraceResult& out) const;
		};


		/**
		 * @brief レイと矩形の交差
		 * @param origin レイの始点
		 * @param direction レイの向き（正規化済み）
		 * @param rect 矩形
		 * @param outT 当たった距離（始点が矩形の中なら 0）
		 * @return 当たったら true
		 */
		bool RayIntersectRect(const math::Vec2& origin, const math::Vec2& direction, const math::Rect& rect, float& outT);
		/**
		 * @brief レイと線分の交差
		 * @param outT 当たった距離
		 * @return 当たったら true（始点ぴったりの交差は除く）
		 */
		bool RayIntersectSegment(const math::Vec2& origin, const math::Vec2& direction, const math::Vec2& a, const math::Vec2& b, float& outT);
	}
}
