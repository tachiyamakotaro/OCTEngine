/**
 * @file FakeWall.h
 * @brief 偽物の壁（反射した光が当たると消えて、通り抜けられる。手鏡の視界でも消える）
 */
#pragma once
#include <string>
#include <vector>
#include "RectGimmick.h"
#include "HandMirror.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 光が当たったとき、偽物の壁のどこが消えるか
		 */
		enum class FakeWallLightMode
		{
			Whole,	//!< 壁ごと全部消える
			Hole,	//!< 光が壁を通る道の真ん中を中心に、丸い穴が開く（半径 lightHoleRadius）
			Band,	//!< 光が通る道に沿って、帯の形に消える（太さの半分 lightBandRadius）
		};


		/**
		 * @brief 光で開いた穴（線分 a-b からの距離が radius 以内が消える。a == b なら丸）
		 */
		struct FakeWallLightHole
		{
			math::Vec2 a;
			math::Vec2 b;
			float radius = 0.0f;
		};


		/**
		 * @brief 偽物の壁
		 * @details ・ふだんは本物の地形ブロック（StageBlock）と同じ見た目で、同じようにぶつかる。
		 *          ・反射鏡で反射した光が当たると、lightMode の形に消える（光が外れたらすぐ戻る）。
		 *            消えた所は光も通り抜けるので、光はそのまま奥へ進む（通り抜けた光でも「当たっている」と数えるので、消えたまま）。
		 *          ・手鏡を構えると、視界（扇形）に入った部分だけ消えて見える（見た目は FakeWallRenderer と fakeWall.fx）。
		 *            視界を外しても、当てていた所はしばらく消えたままで、時間がたつと元に戻る（HandMirror::CollectReveals）。
		 *          ・当たり判定は、壁を collisionCellSize の大きさのマスに分けて、消えている範囲（消えている度合いが passRevealRate 以上）に
		 *            中心が入っていないマスだけ残す。だから、消えている所だけ通り抜けられる（QueryCollision）。
		 *          ・消えていた所が元に戻ったとき、キャラクターが中にいたら、体と同じ高さの所を壁の端から端まで空けておく
		 *            （壁に埋まって動けなくならないように。横に歩けば出られる）。壁から出たら元に戻る。
		 *          ・光も同じく、消えている所は通り抜け、消えていない所に当たったら止まる。
		 */
		class FakeWall : public RectGimmick
		{
		public:
			/** 消えている度合いがこれ以上の範囲なら通り抜けられる（戻りかけで半分ほど見えてきたら、もう通れない） */
			float passRevealRate = 0.5f;
			/** 当たり判定のマスの大きさ（小さいほど見た目に近いが、少し重くなる） */
			float collisionCellSize = 10.0f;
			/** 光が当たったとき、どこが消えるか */
			FakeWallLightMode lightMode = FakeWallLightMode::Whole;
			/** 反射した光だけで消えるか（false なら光源からまっすぐ来た光でも消える） */
			bool isReflectedLightOnly = true;
			/** Hole：穴の半径（体が通れるように、体の高さの半分より少し大きく。壁の厚みの半分より大きくすると向こうまで抜ける） */
			float lightHoleRadius = 90.0f;
			/** Band：帯の太さの半分（光が太いときは、光の太さの半分のほうが大きければそちらを使う → 光が当たっている所がそのまま消える） */
			float lightBandRadius = 20.0f;

			/** 1つの壁に開けられる光の穴の数の上限（fakeWall.fx の LIGHT_HOLE_MAX 以下にする） */
			static constexpr int MAX_LIGHT_HOLES = 8;

			/** 【実行時】今、どこか1か所でも消えているか */
			bool isPassable = false;
			/** 【実行時】消えている範囲（当たり判定と光の判定に使う） */
			std::vector<HandMirrorReveal> reveals;
			/** 【実行時】元に戻った壁の中にいるキャラクターのために、空けておく所 */
			std::vector<math::Rect> holdOpenAreas;
			/** 【実行時】光が当たっているか */
			bool isLitByLight = false;
			/** 【実行時】光で開いた穴（Hole と Band のとき） */
			std::vector<FakeWallLightHole> lightHoles;


		public:
			/** @brief 光で壁ごと消えているか（見た目では壁を描かない） */
			bool IsWhollyRevealed() const { return lightMode == FakeWallLightMode::Whole && isLitByLight; }


		public:
			const char* GetTypeName() const override { return "FakeWall"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<FakeWall>(*this); }

			/** 手鏡を拾ったあと（Input のあと）に調べる */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Late; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			/** 壁全体の矩形（どこも消えていないときだけ。実際の判定は QueryCollision） */
			CollisionKind GetCollision(math::Rect& outRect) const override;
			/** 消えていないマスのうち、box と重なる物を返す */
			CollisionKind QueryCollision(const math::Rect& box, std::vector<math::Rect>* out) const override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			/** 地形ブロックと一緒に、見取り図の一番後ろに描く */
			EditorLayer GetEditorLayer() const override { return EditorLayer::Back; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		private:
			/** @brief 光の線分を見て、光が当たっているかと、開いた穴を決める */
			void UpdateLightReveal(const LightTraceResult& light);
			/** @brief 点が、通り抜けられるほど消えている範囲に入っているか */
			bool IsPassableAt(const math::Vec2& point) const;
			/** @brief 矩形が、通り抜けられるほど消えている範囲に少しでも入っているか */
			bool IsPassableOverlap(const math::Rect& area) const;
			/** @brief マスの大きさ（小さすぎるとマスが増えすぎるので下限をつける） */
			float GetCellSize() const;
			/**
			 * @brief box と重なる、消えていないマスを集める（横に並んだマスは1つの矩形にまとめる）
			 * @param out 足す先（nullptr なら、1つでもあるかだけ調べる）
			 * @param isUseHold holdOpenAreas と重なるマスを空けておくか
			 * @return 1つでもあれば true
			 */
			bool CollectSolidCells(const math::Rect& box, std::vector<math::Rect>* out, bool isUseHold) const;


		public:
			/** x, y は左下 */
			FakeWall(const char* name, float x, float y, float width, float height);
		};
	}
}
