/**
 * @file FloorSwitch.h
 * @brief 床スイッチ（キャラクターが踏むと押される）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 床スイッチ
		 * @details 当たり判定はない（台の上に薄く置く飾りの板）。キャラクターの体が板と重なると押される。
		 *          押されている間、自分の名前の信号が ON になる。
		 */
		class FloorSwitch : public RectGimmick
		{
		public:
			/** 一度踏んだら押されたままにするか */
			bool isLatch = true;

			/** 【実行時】押されているか */
			bool isPressed = false;


		public:
			const char* GetTypeName() const override { return "FloorSwitch"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<FloorSwitch>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Input; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override { isPressed = false; }

			bool IsSignalSource() const override { return true; }
			bool IsSignalOn() const override { return isPressed; }

			/** 薄いので、上下に少し広めに拾う */
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** x, y は左下（台の上面に薄く置く） */
			FloorSwitch(const char* name, float x, float y, float width, float height);
		};
	}
}
