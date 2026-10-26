/**
 * @file CharacterParameter.h
 * @brief キャラクター共通のパラメーター（調整値）
 * @details ゲーム中に変わらない「調整用の数値」をまとめる。
 *          ゲーム中に変わる値（HPなど）は CharacterStatus に置く。
 *          JSON から読み込みたくなったら、以前のプロジェクトの IMasterParameter と同じ形にできる。
 *
 *          単位：長さは k2EngineLow のワールド単位（unityChan.tkm の実測で身長 ≒ 119）、時間は秒。
 */
#pragma once


namespace app
{
	namespace actor
	{
		/**
		 * @brief キャラクター共通パラメーター
		 */
		struct CharacterParameter
		{
			/** 体の幅 */
			float bodyWidth = 50.0f;
			/** 体の高さ */
			float bodyHeight = 120.0f;

			/** 走る速さ */
			float runSpeed = 400.0f;
			/** 地上での加速度（最高速に向かう速さ） */
			float groundAcceleration = 3000.0f;
			/** 地上での減速度（入力をやめた/切り返したときに止まる速さ） */
			float groundDeceleration = 4000.0f;
			/** 空中での加速度 */
			float airAcceleration = 1500.0f;
			/** 空中での減速度 */
			float airDeceleration = 800.0f;

			/** 重力（下向きなので負の値） */
			float gravity = -3000.0f;
			/** 落下速度の上限 */
			float maxFallSpeed = 1600.0f;

			/** 最大HP */
			int maxHp = 3;

			/**
			 * @brief モデルの向きの補正（度）
			 * @details 右を向いたときにモデルが逆を向いていたら 180 にする
			 */
			float modelYawOffsetDeg = 0.0f;

			/**
			 * @brief モデルの拡大率
			 * @details モデルの見た目の身長を、体の高さ（bodyHeight）に合わせる。当たり判定の大きさは変わらない
			 */
			float modelScale = 1.0f;

			virtual ~CharacterParameter() = default;
		};
	}
}
