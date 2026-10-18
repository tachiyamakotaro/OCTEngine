/**
 * @file LightReceiver.h
 * @brief 受光器（光が当たっている間「点灯」する）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 受光器
		 * @details 光はここで止まる。点灯している間、自分の名前の信号が ON になる（扉や霧のきっかけに使う）。
		 */
		class LightReceiver : public RectGimmick
		{
		public:
			/** 【実行時】光が当たっているか */
			bool isLit = false;


		public:
			const char* GetTypeName() const override { return "LightReceiver"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<LightReceiver>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override { isLit = false; }

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			bool IsSignalSource() const override { return true; }
			bool IsSignalOn() const override { return isLit; }

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;


		public:
			/** x, y は左下 */
			LightReceiver(const char* name, float x, float y, float width, float height);
		};
	}
}
