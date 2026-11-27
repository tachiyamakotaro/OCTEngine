/**
 * @file UIGroupTransform.h
 * @brief まとまった UI（ウィンドウとその中身など）全体にかける変形（拡大率・濃さ）
 * @details ウィンドウを拡大縮小で出し入れするときに、ウィンドウの中の文字や札も一緒に大きさ・濃さを変えるために使う。
 *          pivot（拡大の中心）を真ん中に、位置は pivot からの距離に拡大率をかけた所へ動かし、
 *          画像の拡大率と色の透明度にも、まとめた拡大率・濃さをかける。
 *          例：pivot = (0, 70)、scale = 0.5 のとき、(400, 70) に置く文字は (200, 70) に半分の大きさで出る。
 *          座標は画面の真ん中が原点、上が +Y（画面は 1920x1080）。
 */
#pragma once


namespace app
{
	namespace ui
	{
		/**
		 * @brief まとまった UI 全体にかける変形
		 */
		struct UIGroupTransform
		{
			/** 拡大の中心 */
			Vector2 pivot;
			/** 拡大率（1 = そのまま） */
			float scale = 1.0f;
			/** 濃さ（0 = 見えない 〜 1） */
			float alpha = 1.0f;

			/** 何もしない変形（拡大率 1・濃さ 1） */
			static UIGroupTransform Identity()
			{
				UIGroupTransform group;
				group.pivot = Vector2(0.0f, 0.0f);
				return group;
			}

			/** 拡大率と濃さはそのままで、拡大の中心だけ変えた物（別の場所にある小さなウィンドウを、その場で拡大縮小させたいとき） */
			UIGroupTransform WithPivot(const Vector2& newPivot) const
			{
				UIGroupTransform group = *this;
				group.pivot = newPivot;
				return group;
			}

			/** 位置にかける（pivot からの距離に拡大率をかける） */
			Vector2 ApplyPosition(const Vector2& position) const
			{
				return Vector2(pivot.x + (position.x - pivot.x) * scale, pivot.y + (position.y - pivot.y) * scale);
			}

			/** 画像の拡大率にかける */
			float ApplyScale(const float spriteScale) const
			{
				return spriteScale * scale;
			}

			/** 色の透明度にかける */
			Vector4 ApplyColor(const Vector4& color) const
			{
				return Vector4(color.x, color.y, color.z, color.w * alpha);
			}
		};
	}
}
