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
		 * @brief 氷が溶けていく面
		 */
		enum class IceMeltSide
		{
			Top,	//!< 上から低くなる（上から光が当たった足場など）
			Bottom,	//!< 下から縮む
			Left,	//!< 左から薄くなる（左から光が当たった壁など）
			Right,	//!< 右から薄くなる
		};


		/**
		 * @brief 氷
		 * @details ・光が当たっている間だけ溶けていき、meltTime 秒当たり続けると溶けきって消える。
		 *          ・日陰（光が届かない所）にある間は溶けない。溶けた分は元に戻らない（リスタートで戻る）。
		 *          ・最初に光が当たった面から溶けていく（左から光が当たった壁は左から薄くなり、
		 *            上から光が当たった足場は上から低くなる）。当たり判定も光を止める範囲も今の大きさに合わせる。
		 *            ※ いつも上から低くしていくと、横から当たる低い光が途中で氷の上を通り抜けてしまい、溶けきらないため
		 *          ・氷は光を止める（Absorb）。氷の後ろにある物は、氷が溶けるまで日陰になる。
		 */
		class IceBlock : public RectGimmick
		{
		public:
			/** 種類（見た目と役割） */
			IceKind kind = IceKind::Melt;
			/** 光が当たり続けて溶けきるまでの時間（秒） */
			float meltTime = 2.0f;

			/** 【実行時】溶けた割合（0=元の大きさ 〜 1=溶けきった） */
			float meltAmount = 0.0f;
			/** 【実行時】今フレーム光が当たっているか（溶けている最中か） */
			bool isLit = false;
			/** 【実行時】このフレームに溶けきったか（湯気のエフェクトや音を出すきっかけに使う） */
			bool isMeltedThisFrame = false;
			/** 【実行時】溶けていく面（最初に光が当たったときに決まる） */
			IceMeltSide meltSide = IceMeltSide::Top;


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
			/** 溶けきったか */
			bool IsMelted() const { return meltAmount >= 1.0f; }
			/** 今の大きさ（溶けた分だけ、溶けていく面のほうから縮む） */
			math::Rect GetCurrentRect() const;


		private:
			/** 光の線分の終わりの点を見て、今の大きさのどの面に光が当たっているかを調べる */
			IceMeltSide FindLitSide(const LightTraceResult& light) const;


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
