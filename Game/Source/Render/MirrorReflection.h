/**
 * @file MirrorReflection.h
 * @brief Z = planeZ の平面鏡に映したときの、位置・回転・拡大率の計算
 * @details 奥の壁の鏡に映る像は、「同じモデルをもう1つ、鏡に映した位置・向き・拡大率で置く」ことで描く。
 *          （本物の鏡と同じく、正しく左右が反転した像になる）
 *
 *          モデルのワールド行列は W = S * R * T（拡大 → 回転 → 移動）。
 *          これに Z 反転 M をかけた W * M を、同じ S' * R' * T' の形に直すと
 *            S' = (sx, sy, -sz)        … Z だけ拡大率を負にする
 *            R' = (-qx, -qy, qz, qw)   … 回転の X・Y 成分の符号を反転する（Y軸回転なら角度が逆になる）
 *            T' = (tx, ty, 2*planeZ - tz)
 *          になる。拡大率が負だとポリゴンの表裏が逆になるので、
 *          映り込み用のモデルは表側を消す（D3D12_CULL_MODE_FRONT）設定で作ること。
 */
#pragma once


namespace app
{
	namespace render
	{
		/** 位置を鏡に映す */
		inline Vector3 ReflectPositionZ(const Vector3& position, const float planeZ)
		{
			return Vector3(position.x, position.y, 2.0f * planeZ - position.z);
		}


		/** 回転を鏡に映す */
		inline Quaternion ReflectRotationZ(const Quaternion& rotation)
		{
			Quaternion reflected = rotation;
			reflected.x = -rotation.x;
			reflected.y = -rotation.y;
			return reflected;
		}


		/** 拡大率を鏡に映す */
		inline Vector3 ReflectScaleZ(const Vector3& scale)
		{
			return Vector3(scale.x, scale.y, -scale.z);
		}
	}
}
