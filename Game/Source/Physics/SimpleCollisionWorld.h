/**
 * @file SimpleCollisionWorld.h
 * @brief 矩形のリストで地形を表す、いちばんシンプルな当たり判定ワールド
 * @details ブロックを積んで作る横スクロールのステージ向け。
 *          レベル（tkl）に置いた箱オブジェクトの座標と拡大率から AddBlockFromCenter() で登録できる。
 *          ブロックが数千個を超えて重くなったら、グリッド分割した実装に差し替えること。
 */
#pragma once
#include "ICollisionWorld.h"


namespace app
{
	namespace physics
	{
		/**
		 * @brief 矩形リストの当たり判定ワールド
		 */
		class SimpleCollisionWorld : public ICollisionWorld
		{
		public:
			/**
			 * @brief 固い地形を追加する
			 */
			void AddSolid(const math::Rect& solid) { m_solids.push_back(solid); }
			/**
			 * @brief 左下座標とサイズでブロックを追加する
			 */
			void AddBlock(float x, float y, float width, float height);
			/**
			 * @brief 中心座標とサイズでブロックを追加する（レベルのオブジェクトから作るとき用）
			 */
			void AddBlockFromCenter(const math::Vec2& center, const math::Vec2& size);
			/**
			 * @brief 全ての地形を消す
			 */
			void Clear() { m_solids.clear(); }
			/**
			 * @brief 登録されている地形を取得（デバッグ描画用）
			 */
			const std::vector<math::Rect>& GetSolids() const { return m_solids; }


		public:
			void QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const override;
			bool AnyOverlap(const math::Rect& box) const override;


		private:
			/** 固い地形の一覧 */
			std::vector<math::Rect> m_solids;
		};
	}
}
