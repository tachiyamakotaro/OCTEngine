/**
 * @file ICollisionWorld.h
 * @brief キャラクターが問い合わせる「地形の当たり判定」のインターフェース
 * @details CharacterController はこのインターフェースだけを見て移動・接地・崖つかまりを判定する。
 *          地形の持ち方（矩形リスト / タイルマップ / 鏡の中の世界 など）を差し替えても
 *          キャラクター側のコードは変えなくて済む。
 */
#pragma once
#include <vector>
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace physics
	{
		/**
		 * @brief 地形の当たり判定ワールド
		 */
		class ICollisionWorld
		{
		public:
			virtual ~ICollisionWorld() = default;


		public:
			/**
			 * @brief box と重なっている固い地形をすべて out に追加する
			 * @param box 調べる範囲
			 * @param out 結果の追加先（クリアはしない）
			 */
			virtual void QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const = 0;

			/**
			 * @brief box と重なる地形が1つでもあるか
			 * @details 派生クラスで早期リターンする実装に置き換えると速くなる
			 */
			virtual bool AnyOverlap(const math::Rect& box) const
			{
				std::vector<math::Rect> hits;
				QueryOverlap(box, hits);
				return !hits.empty();
			}

			/**
			 * @brief このフレームで、足元の床（動く床）がどれだけ動いたか
			 * @details 動く床がないワールドでは何もしない（false を返す）。
			 * @param groundProbe 足元のすぐ下の薄い箱（前のフレームの位置で調べる）
			 * @param outDelta 床が動いた量
			 * @return 動く床の上に立っていたら true
			 */
			virtual bool GetCarryDelta(const math::Rect& groundProbe, math::Vec2& outDelta) const
			{
				return false;
			}

			/**
			 * @brief box の範囲に地形がないか
			 */
			bool IsBoxFree(const math::Rect& box) const
			{
				return !AnyOverlap(box);
			}
		};
	}
}
