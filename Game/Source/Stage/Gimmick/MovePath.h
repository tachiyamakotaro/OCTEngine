/**
 * @file MovePath.h
 * @brief 「始点と終点の間を動く」動きの部品
 * @details 動く床・動く壁（MovingBlock）が持っている。ギミックそのものではなく、ギミックに持たせる部品。
 *          ほかのギミック（反射鏡・受光器など）を動かしたくなったら、そのクラスに MovePath を1つ持たせて、
 *          ・Update で movePath.Update(動いてよいか, deltaTime)
 *          ・当たり判定や描画の位置に movePath.GetOffset() を足す
 *          ・DrawInspector / Serialize / ResetRuntimeState / Rotate90 から movePath の同じ関数を呼ぶ
 *          とするだけで動かせる。
 *
 *          置いた位置（始点）は動かさずに、「始点からどれだけずれているか」だけを返す。
 *          こうしておくと、遊んでいる途中で「Save JSON」を押しても、動いた先の位置が保存されてしまわない。
 */
#pragma once
#include "Source/Core/Math/Vec2.h"


namespace app
{
	namespace stage
	{
		class GimmickArchive;
		class EditorCanvas;


		/**
		 * @brief 始点と終点の間を動く動き
		 */
		struct MovePath
		{
			/**
			 * @brief 動き方
			 */
			enum class Mode : int
			{
				PingPong = 0,	//!< 往復する（終点に着いたら始点へ戻り、またくり返す）
				OneWay = 1,		//!< 終点まで動いたら止まる
				Num
			};


			// ---- 設定（JSON に保存する）----
			/** 終点が始点からどれだけずれているか（上に 200 動くなら (0, 200)） */
			math::Vec2 offset = math::Vec2(0.0f, 200.0f);
			/** 動く速さ（1秒あたりの距離） */
			float speed = 150.0f;
			/** 動き方 */
			Mode mode = Mode::PingPong;
			/** 往復のとき、端で止まる時間（秒） */
			float waitTime = 0.5f;

			// ---- 実行時（保存しない）----
			/** 【実行時】どこまで動いたか（0=始点 〜 1=終点） */
			float progress = 0.0f;
			/** 【実行時】向かっている方向（1=終点へ、-1=始点へ） */
			float direction = 1.0f;
			/** 【実行時】端で止まっている残り時間 */
			float waitTimer = 0.0f;


			/**
			 * @brief 動かす（毎フレーム）
			 * @param isPowered 動いてよいか（ソーラーパネルに光が当たっている間など）。false の間はその場で止まる
			 * @param deltaTime 経過時間
			 */
			void Update(bool isPowered, float deltaTime);
			/** @brief 実行時の状態を最初（始点）に戻す */
			void ResetRuntimeState();
			/** @brief 今、始点からどれだけずれているか */
			math::Vec2 GetOffset() const { return offset * progress; }
			/** @brief 始点から終点までの距離 */
			float GetLength() const { return offset.Length(); }

			/** @brief 90度ずつ回す（ギミックを回したとき、動く向きも一緒に回す） */
			void Rotate90(int quarterTurns);
			/** @brief プロパティ欄（ImGui） */
			void DrawInspector();
			/** @brief JSON への書き出し・読み込み */
			void Serialize(GimmickArchive& archive);
			/**
			 * @brief 見取り図に道のり（始点の中心から終点の中心への線）を描く
			 * @param startCenter 始点にいるときの中心
			 */
			void DrawEditor(const EditorCanvas& canvas, const math::Vec2& startCenter, unsigned int color) const;
		};
	}
}
