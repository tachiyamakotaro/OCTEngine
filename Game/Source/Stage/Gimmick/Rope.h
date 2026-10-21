/**
 * @file Rope.h
 * @brief ロープ（虫眼鏡で集めた光で燃やすと切れる。吊るしていた物が落ちる）
 */
#pragma once
#include "BurnableGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ロープ
		 * @details ・細い矩形で置く（縦に置けば垂れたロープ、横に置けば張ったロープ）。当たり判定はなく、光も通り抜ける。
		 *          ・燃え尽きると自分の名前の信号が ON になる。吊るされた板（HangingBlock）の ropes にこの名前を入れておくと、
		 *            燃え尽きたときに板が落ちる。
		 *          ・つないだロープ（端が触れているロープ）や、触れている草に燃え広がる。火は草より速く走る。
		 */
		class Rope : public BurnableGimmick
		{
		public:
			const char* GetTypeName() const override { return "Rope"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<Rope>(*this); }


		protected:
			/** 細いので、焦点が少し外れていても温まる */
			float GetHotSpotMargin() const override { return 10.0f; }
			unsigned int GetEditorColor() const override;


		public:
			/** x, y は左下 */
			Rope(const char* name, float x, float y, float width, float height);
		};
	}
}
