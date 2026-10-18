/**
 * @file WarpZone.h
 * @brief ワープ（体の中心が入ると、別の場所へ飛ばす）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ワープ
		 * @details マリオのお城のような「間違った道を進むと最初に戻される」仕掛けや、正解の道からゴールへ運ぶのに使う。
		 *          飛んだあとも走る勢いはそのまま残る。
		 *          disabledBySignal に信号の名前を入れると、その信号がONの間はワープしない（謎を解くと通れる道などに使う）。
		 */
		class WarpZone : public RectGimmick
		{
		public:
			/** 飛ぶ先（足元） */
			math::Vec2 destination;
			/** この信号がONの間はワープしない（空ならいつでもワープする） */
			std::string disabledBySignal;


		public:
			const char* GetTypeName() const override { return "WarpZone"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<WarpZone>(*this); }

			void RenameSignal(const std::string& oldName, const std::string& newName) override;
			void RemoveSignal(const std::string& signalName) override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** x, y は左下、destX, destY は飛ぶ先（足元） */
			WarpZone(const char* name, float x, float y, float width, float height, float destX, float destY);
		};
	}
}
