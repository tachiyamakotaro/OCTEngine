/**
 * @file Stage.h
 * @brief ステージ（ギミックの入れ物・光の計算・奥の鏡）
 * @details ・ICollisionWorld を実装しているので、そのままプレイヤーの当たり判定に渡せる。
 *            当たり判定 = 全ギミックの GetCollision（ブロック・閉じた扉・出ている光の足場・透明の足場）
 *          ・Update() で 踏まれる物 → 光 → 光を受ける物 → 信号を見る物 → 最後 の順に更新する（UpdatePhase）。
 *          ・ギミックの種類ごとの処理は書かない。全部 GimmickManager と各ギミックの仮想関数に任せる。
 *          ・BackMirror はステージの奥の壁の大きな鏡。部屋とプレイヤーを映す（見た目だけ）。
 *
 *          作り方：JSON ファイル（Assets/stage/StageXX_Object.json / StageXX_Gimmick.json）に書いておき、
 *          LoadStageFromJson で読む（StageLoader.h）。StageEditor（ImGui）で位置を調整して「Save JSON」で保存する。
 *          C++ から直接 stage.Add<StageBlock>(...) と並べて作ることもできる。
 */
#pragma once
#include <string>
#include <utility>
#include <vector>
#include "LightTracer.h"
#include "StageObjects.h"
#include "Gimmick/GimmickManager.h"
#include "Source/Physics/ICollisionWorld.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ステージ
		 */
		class Stage : public physics::ICollisionWorld
		{
		public:
			// ---- 定義 ----
			/** @brief 名前を設定 */
			inline void SetName(const std::string& name) { m_name = name; }
			inline const std::string& GetName() const { return m_name; }
			/** @brief プレイヤーの開始位置（足元） */
			inline void SetPlayerStart(const math::Vec2& position) { m_playerStart = position; }
			inline const math::Vec2& GetPlayerStart() const { return m_playerStart; }
			/**
			 * @brief ギミックを追加する
			 * @details 例：auto& door = stage.Add<Door>("Door1", 900.0f, 0.0f, 40.0f, 300.0f);
			 *          引数はそのクラスのコンストラクタにそのまま渡す。返す参照はそのギミックを消すまで使える。
			 */
			template<class T, class... Args>
			T& Add(Args&&... args) { return m_gimmicks.Add<T>(std::forward<Args>(args)...); }
			/**
			 * @brief 奥の鏡を設定
			 * @param enabled 使うか
			 * @param planeZ 鏡の面のZ座標（床の奥端に合わせる）
			 * @param x, y 鏡の枠の左下
			 * @param width, height 鏡の枠の大きさ
			 */
			void SetBackMirror(bool enabled, float planeZ, float x, float y, float width, float height);
			/** @brief 全部消す */
			void Clear();
			/** @brief 実行時の状態（扉の開き具合・点灯など）を最初に戻す */
			void ResetRuntimeState();


		public:
			// ---- 更新 ----
			/**
			 * @brief ギミックを更新する（毎フレーム）
			 * @param deltaTime 経過時間
			 * @param bodies スイッチを踏める体（キャラクターの当たり判定の箱）
			 */
			void Update(float deltaTime, const std::vector<math::Rect>& bodies);
			/** @brief スイッチを踏む体なしで更新する */
			void Update(float deltaTime);


		public:
			// ---- 取得 ----
			/** @brief 全ギミック（種類ごとに欲しいときは GetGimmicks().View<Door>()） */
			inline GimmickManager& GetGimmicks() { return m_gimmicks; }
			inline const GimmickManager& GetGimmicks() const { return m_gimmicks; }
			/** @brief 信号がONか（同じ名前の受光器が点灯している、またはスイッチが押されている） */
			bool IsSignalOn(const std::string& name) const { return m_gimmicks.IsSignalOn(name); }
			/**
			 * @brief 霧の吹き出し口の位置（天井の面の上の点）を集める
			 * @details 霧の範囲の上端に ventSpacing の間隔で並べ、真上に地形（天井）があるところだけ使う。
			 *          天井の穴の下などには吹き出し口を付けない。どこにも天井がなければ全部の位置を使う。
			 */
			void GetMistVentPositions(const MistEmitter& emitter, std::vector<math::Vec2>& out) const;
			/** @brief 最後に計算した光の経路 */
			inline const LightTraceResult& GetLightResult() const { return m_lightResult; }
			/** @brief 光の計算の設定 */
			inline LightTracer& GetLightTracer() { return m_lightTracer; }

			/** @brief 奥の鏡 */
			inline BackMirror& GetBackMirror() { return m_backMirror; }
			inline const BackMirror& GetBackMirror() const { return m_backMirror; }

			/** @brief 体が触れているチェックポイント（なければ nullptr） */
			const Checkpoint* FindCheckpoint(const math::Rect& body) const;
			/** @brief 体の中心が入っている、今使えるワープ（なければ nullptr） */
			const WarpZone* FindWarpZone(const math::Rect& body) const;
			/** @brief 体が触れているゴール（なければ nullptr） */
			const Goal* FindGoal(const math::Rect& body) const;


		public:
			// ---- 当たり判定（ICollisionWorld）----
			void QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const override;
			bool AnyOverlap(const math::Rect& box) const override;
			/**
			 * @brief 地形だけ（ブロック + 閉じた扉 + 透明の足場）と重なるか
			 * @details 反射鏡を動かせるかの判定に使う（光の足場は含めない）
			 */
			bool OverlapsTerrain(const math::Rect& box) const { return m_gimmicks.OverlapsTerrain(box); }


		public:
			Stage() = default;
			~Stage() override = default;


		private:
			/** ステージ名 */
			std::string m_name;
			/** プレイヤーの開始位置（足元） */
			math::Vec2 m_playerStart;
			/** 全ギミック */
			GimmickManager m_gimmicks;

			/** 奥の鏡 */
			BackMirror m_backMirror;

			/** 光の計算 */
			LightTracer m_lightTracer;
			/** 光の計算結果 */
			LightTraceResult m_lightResult;
		};
	}
}
