/**
 * @file MirroredCollisionWorld.cpp
 * @brief 別の当たり判定ワールドを、鏡の面で左右反転して見せるワールド
 */
#include "stdafx.h"
#include "MirroredCollisionWorld.h"


namespace app
{
	namespace physics
	{
		math::Rect MirroredCollisionWorld::Reflect(const math::Rect& rect) const
		{
			// 左右が入れ替わるので min と max も入れ替える
			return { { ReflectX(rect.max.x), rect.min.y }, { ReflectX(rect.min.x), rect.max.y } };
		}


		void MirroredCollisionWorld::QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const
		{
			if (m_source == nullptr)
			{
				return;
			}

			// 調べる範囲を元の世界に映して問い合わせ、結果を鏡の中に映し戻す
			const size_t firstIndex = out.size();
			m_source->QueryOverlap(Reflect(box), out);
			for (size_t i = firstIndex; i < out.size(); ++i)
			{
				out[i] = Reflect(out[i]);
			}
		}


		bool MirroredCollisionWorld::AnyOverlap(const math::Rect& box) const
		{
			return m_source != nullptr && m_source->AnyOverlap(Reflect(box));
		}


		MirroredCollisionWorld::MirroredCollisionWorld(const ICollisionWorld* source, const float planeX)
			: m_source(source)
			, m_planeX(planeX)
		{}
	}
}
