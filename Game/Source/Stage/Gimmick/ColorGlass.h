/**
 * @file ColorGlass.h
 * @brief 色ガラス（通った光の色が変わる）
 * @details 外すときは、StageObjects.h の include・GimmickRegistry.cpp のボタン・DebugScene の ColorGlassRenderer を消して、このファイルを消す。
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 色ガラス
		 * @details 現実の色ガラス（色セロハン）と同じく、ガラスと同じ色の光だけを通し、ほかの色は吸収する。
		 *            白い光   → 赤いガラス → 赤い光
		 *            黄色の光 → 赤いガラス → 赤い光（黄 = 赤 + 緑 の、赤だけ残る）
		 *            青い光   → 赤いガラス → 光が消える（そこで止まる）
		 *          光はまっすぐ通り抜ける（曲がらない）。当たり判定は isSolid で選ぶ（ふつうはなし）。
		 *          光の計算は LightResponse::Filter で行う（LightTracer.cpp）。
		 */
		class ColorGlass : public RectGimmick
		{
		public:
			/** ガラスの色（通った光は、この色の成分だけ残る） */
			LightColor color = LightColor::Red;
			/** プレイヤーがぶつかるか（ガラスの壁にするなら true） */
			bool isSolid = false;

			/** 【実行時】光が当たっているか */
			bool isLit = false;
			/** 【実行時】ガラスに入ってきた光の色（何本も当たっていれば重ねた色） */
			LightColor incomingColor = LightColor::None;


		public:
			const char* GetTypeName() const override { return "ColorGlass"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<ColorGlass>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** ガラスを通ったあとの光の色 */
			LightColor GetOutgoingColor() const { return FilterLightColor(incomingColor, color); }


		public:
			/** x, y は左下 */
			ColorGlass(const char* name, float x, float y, float width, float height, LightColor initialColor);
		};
	}
}
