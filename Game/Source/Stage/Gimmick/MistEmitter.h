/**
 * @file MistEmitter.h
 * @brief 霧の発生装置（天井から霧を吹き出す）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 霧の発生装置
		 * @details rect は霧が広がる範囲（上端が天井＝吹き出し口の高さ）。
		 *          信号（triggerSignal）がONになると一度だけ吹き出す。
		 *          霧は rect の上端から下へ fallSpeed で広がり、duration 秒吹いたあと fadeTime 秒で晴れる。
		 *          霧が触れた透明の足場（InvisibleFloor）は見えるようになる。
		 */
		class MistEmitter : public RectGimmick
		{
		public:
			/** 吹き出すきっかけの信号（スイッチや受光器の名前） */
			std::string triggerSignal;
			/** 霧が下へ広がる速さ（1秒あたり） */
			float fallSpeed = 500.0f;
			/** 吹き出している時間 */
			float duration = 3.0f;
			/** 吹き終わってから晴れるまでの時間 */
			float fadeTime = 1.5f;
			/** 吹き出し口の間隔（見た目。天井にこの間隔で並べる） */
			float ventSpacing = 220.0f;

			/** 【実行時】吹き出したか（一度だけ） */
			bool hasFired = false;
			/** 【実行時】吹き出してからの時間 */
			float timer = 0.0f;
			/** 【実行時】このフレームに吹き出したか（エフェクトや音を出すきっかけに使う） */
			bool isFiredThisFrame = false;


		public:
			const char* GetTypeName() const override { return "MistEmitter"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<MistEmitter>(*this); }

			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Signal; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;

			void RenameSignal(const std::string& oldName, const std::string& newName) override;
			void RemoveSignal(const std::string& signalName) override;

			/** 部屋全体を覆うので、枠線の近くをクリックしたときだけ選ぶ */
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 霧が出ているか（吹いている間 + 晴れるまで） */
			bool IsActive() const { return hasFired && timer < duration + fadeTime; }
			/** 霧の先端（いちばん下まで届いている高さ） */
			float GetFrontY() const
			{
				if (!hasFired) { return rect.max.y; }
				return math::Max(rect.min.y, rect.max.y - fallSpeed * timer);
			}
			/** 霧の濃さ（0〜1。吹き始めに濃くなり、吹き終わると薄れて消える） */
			float GetDensity() const;


		public:
			/** x, y, width, height は霧が広がる範囲（上端が天井） */
			MistEmitter(const char* name, float x, float y, float width, float height, const char* initialTriggerSignal);
		};
	}
}
