/**
 * @file IceBlock.h
 * @brief 氷（光が当たっている間だけ少しずつ溶ける）
 */
#pragma once
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 氷の種類（ゲームの動きは同じ。見た目と、ステージの中での役割が違う）
		 */
		enum class IceKind
		{
			Melt,	//!< 溶かす氷：道をふさいでいる。光を当てて溶かすと先へ進める（白っぽい水色・ひび）
			Keep,	//!< 守る氷：足場になっている。溶けると渡れなくなるので、日陰に入れて守る（濃い青・白い縁）
		};


		/**
		 * @brief 氷の面（溶けた厚さ・光が当たっている面の配列の番号に使う）
		 */
		enum class IceMeltSide
		{
			Top,	//!< 上の面（上から光が当たると、上から低くなる）
			Bottom,	//!< 下の面（下から光が当たると、下から縮む）
			Left,	//!< 左の面（左から光が当たると、左から薄くなる）
			Right,	//!< 右の面（右から光が当たると、右から薄くなる）
			Num
		};


		/**
		 * @brief 氷
		 * @details ・光が当たっている間だけ溶けていき、meltTime 秒当たり続けると溶けきって消える。
		 *          ・日陰（光が届かない所）にある間は溶けない。溶けた分は元に戻らない（リスタートで戻る）。
		 *          ・光が当たっている面から溶けていく。左から当たれば左から薄くなり、下から当たれば下から縮む。
		 *            途中で光の向きが変わったら、そのときに当たっている面から溶ける（2方向から当たれば両方から溶ける）。
		 *            当たり判定も光を止める範囲も今の大きさに合わせる。
		 *          ・どの面から溶かしても、meltTime 秒当たり続けると溶けきる（幅の分も高さの分も同じ時間で溶ける速さ）。
		 *          ・氷は光を止める（Absorb）。氷の後ろにある物は、氷が溶けるまで日陰になる。
		 */
		class IceBlock : public RectGimmick
		{
		public:
			/** 種類（見た目と役割） */
			IceKind kind = IceKind::Melt;
			/** 光が当たり続けて溶けきるまでの時間（秒） */
			float meltTime = 2.0f;

			/** 【実行時】それぞれの面から溶けた厚さ（IceMeltSide の順。rect からこの分だけ削った大きさが今の大きさ） */
			float meltedDepth[static_cast<int>(IceMeltSide::Num)] = { 0.0f, 0.0f, 0.0f, 0.0f };
			/** 【実行時】今フレーム光が当たっている面（IceMeltSide の順） */
			bool litSides[static_cast<int>(IceMeltSide::Num)] = { false, false, false, false };
			/** 【実行時】今フレーム光が当たっているか（溶けている最中か） */
			bool isLit = false;
			/** 【実行時】このフレームに溶けきったか（湯気のエフェクトや音を出すきっかけに使う） */
			bool isMeltedThisFrame = false;


		public:
			const char* GetTypeName() const override { return "IceBlock"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<IceBlock>(*this); }

			/** 光の計算の結果を見て溶ける */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			CollisionKind GetCollision(math::Rect& outRect) const override;

			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 溶けきったか（幅か高さがほとんど 0 になった） */
			bool IsMelted() const;
			/** 今の大きさ（rect から、それぞれの面で溶けた厚さを削った大きさ） */
			math::Rect GetCurrentRect() const;
			/** 溶けた割合（0=元の大きさ 〜 1=溶けきった。面積で数える。表示用） */
			float GetMeltAmount() const;


		private:
			/** 光の線分の終わりの点を見て、今の大きさのどの面に光が当たっているかを litSides に入れる（1つでも見つかれば true） */
			bool FindLitSides(const LightTraceResult& light);


		public:
			/** x, y は左下 */
			IceBlock(const char* name, float x, float y, float width, float height, IceKind initialKind);
		};


		/** @brief 種類 → JSON に書く文字（"Melt" / "Keep"） */
		const char* IceKindToString(IceKind kind);
		/** @brief JSON の文字 → 種類（知らない文字なら false で、out は変えない） */
		bool IceKindFromString(const std::string& text, IceKind& out);
	}
}
