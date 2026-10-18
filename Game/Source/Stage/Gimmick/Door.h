/**
 * @file Door.h
 * @brief 扉（つながった信号＝受光器・スイッチが全部ONになると開く）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 扉
		 * @details rect は閉じているときの範囲。開くと下端が上に上がっていく（天井に引っ込むシャッター）。
		 */
		class Door : public RectGimmick
		{
		public:
			/** 開くのに必要な信号の名前（受光器またはスイッチ。全部ONで開く） */
			std::vector<std::string> requiredReceivers;
			/** 開閉の速さ（1秒あたりの割合） */
			float openSpeed = 2.0f;
			/** 一度開いたら開いたままにするか */
			bool isLatch = false;

			/** 【実行時】開き具合（0=閉 〜 1=開） */
			float openAmount = 0.0f;
			/** 【実行時】ラッチで開きっぱなしになったか */
			bool isLatched = false;


		public:
			const char* GetTypeName() const override { return "Door"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<Door>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Signal; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			void RenameSignal(const std::string& oldName, const std::string& newName) override;
			void RemoveSignal(const std::string& signalName) override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 今の当たり判定の範囲 */
			math::Rect GetCurrentRect() const
			{
				math::Rect current = rect;
				current.min.y += rect.Height() * openAmount;
				return current;
			}
			/** 当たり判定があるか（完全に開いたらない） */
			bool IsSolid() const { return openAmount < 1.0f; }


		public:
			/** x, y は閉じているときの左下 */
			Door(const char* name, float x, float y, float width, float height);
		};
	}
}
