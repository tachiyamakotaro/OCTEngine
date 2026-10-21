/**
 * @file Magnifier.h
 * @brief 虫眼鏡（レンズに当たった光を、その先の一点に集める）
 */
#pragma once
#include "GimmickBase.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 虫眼鏡
		 * @details ・レンズは線分（中心 center・直径 diameter）。光はレンズで止まり（Absorb）、
		 *            「光が進んでいた向き」に focalLength 進んだ一点（焦点）に集まる。
		 *            どちらの面から光が当たっても、光が進む側に集まる。
		 *          ・集まった光は、光の計算（LightTracer）には入れない。虫眼鏡が自分で「焦点までに何があるか」を調べる。
		 *            ブロックなど光を止める物が焦点より手前にあると、光はそこで止まり、ぼやけて燃やせない。
		 *          ・焦点（から focusTolerance 以内）に燃える物（BurnableGimmick）があると、その物が温まり、やがて火がつく。
		 *            燃える物は Late の順番で、全部の虫眼鏡の IsHotSpotIn を見て温まる（虫眼鏡は燃える物を知らない）。
		 *          ・angleDeg は「光を通す向き（レンズの軸）」。レンズの面はこれと直角。
		 *            光が当たっていない間は、この向きに「ここに集まる」という目印（うすい光の円すい）を出す。
		 *          ・プレイヤーは動かせない（光の向きは反射鏡で変える）。当たり判定もない。
		 */
		class Magnifier : public GimmickBase
		{
		public:
			/** レンズの中心 */
			math::Vec2 center;
			/** 光を通す向き（レンズの軸。度。0=右へ通す、-90=下へ通す） */
			float angleDeg = 0.0f;
			/** レンズの直径 */
			float diameter = 100.0f;
			/** 焦点までの距離（レンズの中心から） */
			float focalLength = 200.0f;
			/** 焦点のずれをどこまで許すか（燃える物の面が「焦点 ± この距離」にあれば燃やせる） */
			float focusTolerance = 30.0f;
			/** 光が当たっていない間も、集まる所をうすく見せるか */
			bool isShowGuide = true;

			/** 【実行時】光が当たっているか */
			bool isLit = false;
			/** 【実行時】集まった光が進む向き（光が当たっていない間はレンズの軸） */
			math::Vec2 focusDirection = math::Vec2(1.0f, 0.0f);
			/** 【実行時】レンズの中心から、集まった光の先（焦点、または光を止めた物の面）までの距離 */
			float coneLength = 200.0f;
			/** 【実行時】焦点がちょうど物の上（または空中）に結んでいるか（false なら手前でさえぎられてぼやけている） */
			bool isSharp = true;
			/** 【実行時】レンズの軸の向きに集めたときの焦点（光が当たっていなくても調べる。燃やす所の黒い印に使う） */
			math::Vec2 guidePoint;
			/** 【実行時】軸の向きに集めたとき、焦点がはっきり結ぶか */
			bool isGuideSharp = true;


		public:
			const char* GetTypeName() const override { return "Magnifier"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<Magnifier>(*this); }

			/** 光の計算の結果を見るので Light の順番 */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;

			/** レンズに当たった光は止まる（Absorb。点灯して集める） */
			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			math::Vec2 GetAnchor() const override { return center; }
			void SetAnchor(const math::Vec2& anchor) override { center = anchor; }
			math::Rect GetBounds() const override;
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			EditorLayer GetEditorLayer() const override { return EditorLayer::Front; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;
			/** 光を通す向きを90度ずつ回す */
			void Rotate90(const int quarterTurns) override { angleDeg = math::NormalizeDeg(angleDeg + 90.0f * static_cast<float>(quarterTurns)); }


		public:
			/** レンズの軸（光を通す向き。長さ1） */
			math::Vec2 GetAxis() const { return math::DirectionFromDeg(angleDeg); }
			/** レンズの面に沿った向き（軸を反時計回りに90度回した向き。持ち手はこの逆向きに付いている） */
			math::Vec2 GetLensDirection() const
			{
				const math::Vec2 axis = GetAxis();
				return { -axis.y, axis.x };
			}
			/** レンズの端（持ち手の側） */
			math::Vec2 GetLensStart() const { return center - GetLensDirection() * (diameter * 0.5f); }
			/** レンズの端（持ち手と反対の側） */
			math::Vec2 GetLensEnd() const { return center + GetLensDirection() * (diameter * 0.5f); }
			/** 集まった光の先の点（焦点、またはさえぎった物の面の上） */
			math::Vec2 GetFocusPoint() const { return center + focusDirection * coneLength; }
			/** 物を燃やせる状態か（光が当たっていて、焦点がはっきり結んでいる） */
			bool CanBurn() const { return isLit && isSharp; }
			/**
			 * @brief 集まった光の熱い点が、矩形の中（まわり margin まで）にあるか
			 * @details 燃える物が「自分が温められているか」を調べるのに使う
			 */
			bool IsHotSpotIn(const math::Rect& rect, float margin) const;
			/**
			 * @brief レンズの軸の向きの焦点（光を当てたら燃える所）が、矩形の中（まわり margin まで）にあるか
			 * @details 燃える物が「自分のどこに黒い印を付けるか」を調べるのに使う（光が当たっていなくても調べる）
			 */
			bool IsGuidePointIn(const math::Rect& rect, float margin) const;


		private:
			/**
			 * @brief レンズに当たっている光の向きを、光の線分から探す
			 * @return 見つかったら true
			 */
			bool FindIncomingDirection(const GimmickUpdateContext& context, math::Vec2& outDirection) const;
			/**
			 * @brief レンズの中心から direction の向きに集めた光が、どこまで届くか
			 * @return 焦点の手前で光を止める物があればその面までの距離、なければ focalLength
			 */
			float TraceFocusLength(const GimmickUpdateContext& context, const math::Vec2& direction) const;


		public:
			/** x, y はレンズの中心 */
			Magnifier(const char* name, float x, float y, float initialAngleDeg);
		};
	}
}
