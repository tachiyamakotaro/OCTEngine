/**
 * @file SolarPanel.h
 * @brief ソーラーパネル（光が当たっている間だけ信号が ON になる）
 */
#pragma once
#include "LightReceiver.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ソーラーパネル
		 * @details 光が当たっている間、自分の名前の信号が ON になる（動く床・動く壁をつなぐ）。
		 *          しくみは受光器（LightReceiver）と同じなので、受光器を継承して、名前と見取り図の色だけ変えている。
		 *          受光器として扱われるので、モデルの描画・点灯の SE・デバッグ表示は受光器のものがそのまま使われる。
		 *          （ソーラーパネルのモデルができたら、GimmickRenderer で SolarPanel だけ別のモデルにする）
		 */
		class SolarPanel : public LightReceiver
		{
		public:
			const char* GetTypeName() const override { return "SolarPanel"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<SolarPanel>(*this); }

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;


		public:
			/** x, y は左下 */
			SolarPanel(const char* name, float x, float y, float width, float height);
		};
	}
}
