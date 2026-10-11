/**
 * @file LightTracer.h
 * @brief 光の経路を計算する（レイを飛ばして鏡で反射させる）
 * @details 光源からまっすぐレイを飛ばし、一番近くで当たった物で処理を分ける。
 *          ・反射鏡（表）… 反射して続ける
 *          ・反射鏡（裏）/ ブロック / 閉じた扉 … 止まる
 *          ・受光器 … 止まって、その受光器を点灯させる
 *          ・光の足場 … 止まらずに通り抜け、その足場を点灯させる
 */
#pragma once
#include <vector>
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		class Stage;


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
			/** 受光器ごとの点灯（Stage の受光器と同じ並び） */
			std::vector<bool> litReceivers;
			/** 光の足場ごとの点灯（Stage の光の足場と同じ並び） */
			std::vector<bool> litPlatforms;


			void Clear()
			{
				segments.clear();
				litReceivers.clear();
				litPlatforms.clear();
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
			 * @brief ステージの全光源について光の経路を計算する
			 */
			void Trace(const Stage& stage, LightTraceResult& out) const;


		private:
			/**
			 * @brief 1本の光を追いかける
			 */
			void TraceBeam(const Stage& stage, math::Vec2 origin, math::Vec2 direction, LightTraceResult& out) const;
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
