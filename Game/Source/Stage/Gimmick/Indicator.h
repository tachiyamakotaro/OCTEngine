/**
 * @file Indicator.h
 * @brief 合図のランプ（信号がONになると光る。見た目だけ）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 合図のランプ
		 * @details 光の謎を解くと正解の道のランプが光る、など。signal が空のランプはずっと消えたまま（ダミー）。
		 */
		class Indicator : public RectGimmick
		{
		public:
			/** 光るきっかけの信号（受光器・スイッチの名前） */
			std::string signal;

			/** 【実行時】光っているか */
			bool isOn = false;


		public:
			const char* GetTypeName() const override { return "Indicator"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<Indicator>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Signal; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override { isOn = false; }

			void RenameSignal(const std::string& oldName, const std::string& newName) override;
			void RemoveSignal(const std::string& signalName) override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** x, y は左下。initialSignal がONで光る（空ならずっと消えたまま） */
			Indicator(const char* name, float x, float y, float width, float height, const char* initialSignal);
		};
	}
}
