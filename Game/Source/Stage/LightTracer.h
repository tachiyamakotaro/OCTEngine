/**
 * @file LightTracer.h
 * @brief 光の経路を計算する（レイを飛ばして鏡で反射させる）
 * @details 光を出すギミック（GetEmittedRay）からまっすぐレイを飛ばし、
 *          全ギミックの IntersectLight で一番近くに当たった物を探して、その反応で処理を分ける。
 *          ・Reflect     … 反射して続ける（反射鏡）
 *          ・Stop        … 止まる（ブロック・閉じた扉・鏡の裏）
 *          ・Absorb      … 止まって、そのギミックを点灯させる（受光器）
 *          ・PassThrough … 止まらずに通り抜け、そのギミックを点灯させる（光の足場）
 *          ・Filter      … 止まらずに通り抜けるが、そこから先の光の色が変わる（色ガラス）
 *          光には色があり（光源の GetEmittedColor）、点灯したギミックには「どの色の光が当たったか」も記録する。
 *          光の計算はギミックの種類を知らないので、光に反応する新しいギミックを足しても、ここは書き換えなくてよい。
 */
#pragma once
#include <algorithm>
#include <vector>
#include "Source/Core/Math/Rect.h"
#include "LightColor.h"


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
			/** 光の太さ（0 ならふつうの細い光。反射しても光源と同じ太さ） */
			float width = 0.0f;
			/** 始点で反射した鏡の面の向き（光が出ていく側を向く。光源から出た線分は 0） */
			math::Vec2 startNormal;
			/** 終点で反射する鏡の面の向き（光が来た側を向く。鏡で終わらない線分は 0） */
			math::Vec2 endNormal;
			/** 光の色 */
			LightColor color = LightColor::White;
		};


		/**
		 * @brief 光が当たったギミックと、その光の色
		 */
		struct LitColor
		{
			const GimmickBase* gimmick = nullptr;
			LightColor color = LightColor::White;
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
			/** 光が当たったギミックと、その光の色（litGimmicks と同じ順番・同じ数） */
			std::vector<LitColor> litColors;


			/** そのギミックに光が当たったか */
			bool IsLit(const GimmickBase& gimmick) const
			{
				return std::find(litGimmicks.begin(), litGimmicks.end(), &gimmick) != litGimmicks.end();
			}
			/**
			 * @brief そのギミックに当たった光の色（何本も当たっていれば、全部重ねた色。当たっていなければ None）
			 * @details 赤い光と緑の光が両方当たっていれば黄色（光の混色）
			 */
			LightColor GetLitColor(const GimmickBase& gimmick) const
			{
				LightColor mixed = LightColor::None;
				for (const LitColor& lit : litColors)
				{
					if (lit.gimmick == &gimmick)
					{
						mixed = MixLightColor(mixed, lit.color);
					}
				}
				return mixed;
			}
			/** 光が当たったことを記録する */
			void AddLit(const GimmickBase* gimmick, const LightColor color)
			{
				litGimmicks.push_back(gimmick);
				litColors.push_back({ gimmick, color });
			}
			void Clear()
			{
				segments.clear();
				litGimmicks.clear();
				litColors.clear();
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
			 * @param width 光の太さ（線分に記録するだけ。光の進み方には関係しない）
			 * @param color 光の色（色ガラスを通ると変わる）
			 */
			void TraceBeam(const GimmickManager& gimmicks, math::Vec2 origin, math::Vec2 direction, float width, LightColor color, LightTraceResult& out) const;
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
