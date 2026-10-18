/**
 * @file Checkpoint.h
 * @brief チェックポイント（ここに触れると、死んだときにここから再開する）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief チェックポイント
		 * @details 再開するのは範囲の下端の真ん中（床の上に置くと、その床に立った状態で再開する）。
		 *          ステージの仕掛け（扉・スイッチ・霧など）は再開のときに最初の状態に戻る。
		 */
		class Checkpoint : public RectGimmick
		{
		public:
			const char* GetTypeName() const override { return "Checkpoint"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<Checkpoint>(*this); }

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;


		public:
			/** 再開する位置（足元） */
			math::Vec2 GetSpawnPosition() const { return { (rect.min.x + rect.max.x) * 0.5f, rect.min.y }; }


		public:
			/** x, y は左下 */
			Checkpoint(const char* name, float x, float y, float width, float height);
		};
	}
}
