/**
 * @file StageBlock.h
 * @brief 固い地形ブロック
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 固い地形ブロック（乗れる・光を止める）
		 */
		class StageBlock : public RectGimmick
		{
		public:
			const char* GetTypeName() const override { return "StageBlock"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<StageBlock>(*this); }

			CollisionKind GetCollision(math::Rect& outRect) const override;
			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			EditorLayer GetEditorLayer() const override { return EditorLayer::Back; }
			/** 地形のファイルに保存する */
			StageFileKind GetFileKind() const override { return StageFileKind::Object; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;


		public:
			/** x, y は左下 */
			StageBlock(const char* name, float x, float y, float width, float height);
		};
	}
}
