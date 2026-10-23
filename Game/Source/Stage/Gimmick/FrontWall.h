/**
 * @file FrontWall.h
 * @brief 手前の壁（暗い部屋の手前をふさぐ岩の壁。プレイヤーが暗い部屋に入ると、壁全体が透けて中が見える）
 */
#pragma once
#include "RectGimmick.h"
#include <string>


namespace app
{
	namespace stage
	{
		class DarkZone;


		/**
		 * @brief 手前の壁
		 * @details マリオの洞窟のように、明るい所から見ると、暗い部屋の手前に岩の壁がある。
		 *          プレイヤーが暗い部屋に入った瞬間、カメラの手前にある壁全体がすっと透けて、暗い部屋の中が見えるようになる。
		 *          部屋から出ると、また壁が戻ってくる。
		 *          ・暗がり（DarkZone）と名前で紐づける（darkZone）。空なら、壁と重なっている暗がり全部と紐づく。
		 *          ・透ける条件：プレイヤーの体の真ん中が、紐づいた暗がりの中にある
		 *                        または、プレイヤーの体が壁の範囲に入った（壁の後ろにプレイヤーが隠れないように）
		 *          ・見た目だけのギミック。当たり判定はなく、光も止めない。
		 *          ・壁は光やほかの半透明の物よりも手前に描く（DarknessRenderer::DrawFrontWalls）。
		 *            壁がある間は、中の光の筋も見えない（中に入ってから見える）。
		 */
		class FrontWall : public RectGimmick
		{
		public:
			/** 紐づける暗がりの名前（空なら、壁と重なっている暗がり全部） */
			std::string darkZone;
			/** 透ける・戻るまでの秒数 */
			float fadeTime = 0.35f;
			/** 岩の色（0〜1） */
			float color[3] = { 0.30f, 0.38f, 0.55f };
			/** 岩の模様の大きさ（大きいほど、大きな岩になる） */
			float stoneScale = 140.0f;
			/** 壁の端のぼかしの幅（0 ならくっきり） */
			float edgeFeather = 8.0f;
			/** 壁の縁を暗くする幅（立体に見せるため。0 なら暗くしない） */
			float rimWidth = 40.0f;

			/** 【実行時】壁の見え具合（1 = 見えている 〜 0 = 透けて見えない） */
			float visibility = 1.0f;
			/** 【実行時】プレイヤーが中に入っているか */
			bool isPlayerInside = false;


		public:
			const char* GetTypeName() const override { return "FrontWall"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<FrontWall>(*this); }

			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;

			/** 後ろに描き、枠の線の近くだけで選ぶ（中に置いたギミックを選びやすくする） */
			EditorLayer GetEditorLayer() const override { return EditorLayer::Back; }
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** その暗がりと紐づいているか */
			bool IsLinkedTo(const DarkZone& zone) const;


		public:
			/** x, y は左下 */
			FrontWall(const char* name, float x, float y, float width, float height);
		};
	}
}
