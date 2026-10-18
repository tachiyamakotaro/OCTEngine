/**
 * @file RectGimmick.h
 * @brief 矩形の範囲を持つギミックの共通部分
 * @details ほとんどのギミックは「左下とサイズの矩形」で置くので、
 *          位置の移動・プロパティの矩形の欄・JSON の矩形（"position" と "size"）をここでまとめて作っておく。
 *          継承したクラスは、DrawInspector / Serialize の最初で RectGimmick の同じ関数を呼ぶこと。
 */
#pragma once
#include "GimmickBase.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 矩形の範囲を持つギミック
		 */
		class RectGimmick : public GimmickBase
		{
		public:
			/** 範囲（左下とサイズ） */
			math::Rect rect;


		public:
			math::Vec2 GetAnchor() const override { return rect.min; }
			void SetAnchor(const math::Vec2& anchor) override;
			math::Rect GetBounds() const override { return rect; }
			/** 矩形の欄（位置とサイズ） */
			void DrawInspector(GimmickInspector& inspector) override;
			/** JSON の "position": [x, y] と "size": [幅, 高さ] */
			void Serialize(GimmickArchive& archive) override;


		protected:
			/**
			 * @param name 名前
			 * @param x, y 左下
			 * @param width, height 大きさ
			 */
			RectGimmick(const char* name, float x, float y, float width, float height);
		};
	}
}
