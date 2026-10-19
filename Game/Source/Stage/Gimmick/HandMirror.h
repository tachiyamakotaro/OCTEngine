/**
 * @file HandMirror.h
 * @brief 手鏡（拾うと、構えている間だけ扇形の「視界」ができる）
 */
#pragma once
#include <vector>
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 手鏡の視界（扇形）
		 * @details 頂点 origin（手鏡の位置）から direction の向きに、左右 halfAngleDeg ずつ開き、長さ range まで届く。
		 */
		struct HandMirrorCone
		{
			/** 扇形の頂点（手鏡の位置） */
			math::Vec2 origin;
			/** 向き（長さ1） */
			math::Vec2 direction = { 1.0f, 0.0f };
			/** 開きの半分（度） */
			float halfAngleDeg = 30.0f;
			/** 長さ */
			float range = 700.0f;


			/** @brief 点が視界に入っているか */
			bool Contains(const math::Vec2& point) const;
			/** @brief 矩形が少しでも視界に入っているか */
			bool OverlapsRect(const math::Rect& rect) const;
		};


		/**
		 * @brief 偽物の壁が消えている範囲（今の視界と、少し前の視界）
		 */
		struct HandMirrorReveal
		{
			/** 範囲 */
			HandMirrorCone cone;
			/** 消えている度合い（1 = 完全に消えている 〜 0 = 元に戻った） */
			float rate = 1.0f;
		};


		/**
		 * @brief 手鏡
		 * @details ・ステージに置いた範囲（rect）にプレイヤーが触れると拾う。拾ったら置き場所からは消える。
		 *          ・拾ったあとは、構えるボタンを押している間だけ手元に出て、扇形の視界ができる（HandMirrorInteractor が毎フレーム位置と向きを入れる）。
		 *          ・上下の入力で視界を上下に傾けられる（右向きでも左向きでも、上が上）。
		 *          ・視界に入った偽物の壁（FakeWall）は消えて見え、通り抜けられる。
		 *          ・視界を外しても、当てていた所はしばらく（revealKeepTime）消えたままで、そのあと revealRestoreTime かけて元に戻る。
		 *            そのために、構えている間の視界を少しずつ覚えておく（revealHistory）。
		 *          ・拾ったかどうかは、死んで再開しても（ResetRuntimeState でも）残す。JSON を読み直すと最初に戻る。
		 */
		class HandMirror : public RectGimmick
		{
		public:
			/** 消えている範囲の数の上限（今の視界 + 覚えておく視界。fakeWall.fx の REVEAL_MAX と同じにする） */
			static constexpr int MAX_REVEAL_COUNT = 32;

			/** 覚えておく視界1つ */
			struct RevealRecord
			{
				/** その時の視界 */
				HandMirrorCone cone;
				/** 覚えてからの時間 */
				float age = 0.0f;
			};


		public:
			/** 視界の開き（扇形の全体の角度。度） */
			float viewAngleDeg = 60.0f;
			/** 視界の長さ */
			float viewRange = 700.0f;
			/** 上下に傾けられる角度（度） */
			float maxTiltDeg = 60.0f;
			/** 1秒に傾く角度（度） */
			float tiltSpeedDeg = 120.0f;
			/** 視界を外してから、壁が消えたままでいる時間（秒） */
			float revealKeepTime = 2.0f;
			/** そのあと元に戻るまでの時間（秒。ざらざらと戻っていく） */
			float revealRestoreTime = 0.8f;

			/** 【実行時】拾ったか（再開しても持ったまま） */
			bool isCollected = false;
			/** 【実行時】このフレームに拾ったか（音やエフェクトのきっかけに使う） */
			bool isCollectedThisFrame = false;
			/** 【実行時】構えているか */
			bool isRaised = false;
			/** 【実行時】傾き（度。上が正） */
			float tiltDeg = 0.0f;
			/** 【実行時】持っているキャラクターの向き（右なら 1、左なら -1） */
			int facing = 1;
			/** 【実行時】手元の位置（扇形の頂点） */
			math::Vec2 holdPosition;
			/** 【実行時】構えている間に覚えた視界（古い順） */
			std::vector<RevealRecord> revealHistory;
			/** 【実行時】次に視界を覚えるまでの時間 */
			float revealRecordTimer = 0.0f;
			/** 【実行時】前のフレームに構えていたか */
			bool wasViewActive = false;
			/** 【実行時】最後に構えていたときの視界（下ろした瞬間に覚える） */
			HandMirrorCone lastActiveCone;


		public:
			const char* GetTypeName() const override { return "HandMirror"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<HandMirror>(*this); }

			/** プレイヤーに触れられる物なので、光の計算の前に調べる */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Input; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** @brief 視界があるか（拾っていて、構えている） */
			bool IsViewActive() const { return isCollected && isRaised; }
			/** @brief 手鏡が向いている角度（度。0 = 右、90 = 上） */
			float GetAimDeg() const { return facing > 0 ? tiltDeg : 180.0f - tiltDeg; }
			/** @brief 今の視界 */
			HandMirrorCone GetCone() const;
			/**
			 * @brief 傾ける
			 * @param input 上なら 1、下なら -1
			 * @param deltaTime 経過時間
			 */
			void Tilt(float input, float deltaTime);
			/**
			 * @brief 偽物の壁が消えている範囲を out の後ろに足す（構えていれば今の視界、それと覚えている視界）
			 * @details FakeWall（当たり判定・光）と FakeWallRenderer（見た目）が同じ範囲を使う
			 */
			void CollectReveals(std::vector<HandMirrorReveal>& out) const;
			/** @brief 覚えてからの時間 → 消えている度合い（1 〜 0） */
			float GetRevealRate(float age) const;


		private:
			/** @brief 覚えている視界を古くして、構えていれば今の視界を覚える */
			void UpdateRevealHistory(float deltaTime);
			/** @brief 視界を1つ覚える（多すぎたら古いものを消す） */
			void RecordReveal(const HandMirrorCone& cone);


		public:
			/** x, y は左下（拾える範囲） */
			HandMirror(const char* name, float x, float y, float width, float height);
		};
	}
}
