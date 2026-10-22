/**
 * @file ColorReceiver.h
 * @brief 色センサー（決められた色の光が当たっている間だけ「点灯」する受光器）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 色センサー
		 * @details 工場のカラーセンサーのように、当たった光の色を見分ける。
		 *          ・光はどの色でもここで止まる（受光器と同じ）
		 *          ・当たっている光の色が requiredColor とぴったり同じ間だけ、自分の名前の信号が ON になる
		 *            → 扉の requiredReceivers にこの名前を入れると「特定の色の光で開く扉」になる
		 *          ・何本も当たっていると、光を重ねた色で調べる（赤 + 緑 = 黄。黄色のセンサーなら開く、赤のセンサーなら開かない）
		 *          ・白い光は「赤 + 緑 + 青」なので、赤のセンサーは白い光では点灯しない
		 */
		class ColorReceiver : public RectGimmick
		{
		public:
			/** 点灯する光の色 */
			LightColor requiredColor = LightColor::Red;

			/** 【実行時】当たっている光の色（当たっていなければ None） */
			LightColor receivedColor = LightColor::None;
			/** 【実行時】決められた色の光が当たっているか（信号が ON か） */
			bool isMatched = false;


		public:
			const char* GetTypeName() const override { return "ColorReceiver"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<ColorReceiver>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			bool IsSignalSource() const override { return true; }
			bool IsSignalOn() const override { return isMatched; }

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 光が当たっているか（色は問わない） */
			bool IsLit() const { return receivedColor != LightColor::None; }
			/** 違う色の光が当たっているか（見た目で「色が違う」と知らせるのに使う） */
			bool IsWrongColor() const { return IsLit() && !isMatched; }


		public:
			/** x, y は左下 */
			ColorReceiver(const char* name, float x, float y, float width, float height, LightColor initialColor);
		};
	}
}
