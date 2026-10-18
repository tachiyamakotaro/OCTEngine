/**
 * @file Goal.h
 * @brief ゴール（体が触れるとステージクリア）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ゴール
		 */
		class Goal : public RectGimmick
		{
		public:
			const char* GetTypeName() const override { return "Goal"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<Goal>(*this); }

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;


		public:
			/** x, y は左下 */
			Goal(const char* name, float x, float y, float width, float height);
		};
	}
}
