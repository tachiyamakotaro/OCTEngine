/**
 * @file Stage.h
 * @brief ステージ（地形・光のギミック・スイッチと霧・鏡の世界）
 * @details ・ICollisionWorld を実装しているので、そのままプレイヤーの当たり判定に渡せる。
 *            当たり判定 = ブロック + 閉じた扉 + 出ている光の足場 + 鏡の足場
 *          ・Update() で スイッチ → 光 → 受光器・光の足場 → 扉 → 霧 → 鏡の足場 の順に更新する。
 *          ・信号：受光器の点灯とスイッチの押下を、名前で同じように扱う（扉や霧のきっかけに使える）。
 *          ・BackMirror はステージの奥の壁の大きな鏡。部屋とプレイヤーを映す（見た目だけ）。
 *
 *          作り方：C++ で AddBlock() などを並べて定義し（Stage01.cpp 参照）、
 *          StageEditor（ImGui）で位置を調整して、出力した C++ コードを定義に貼り戻す。
 */
#pragma once
#include <string>
#include <vector>
#include "LightTracer.h"
#include "StageObjects.h"
#include "Source/Physics/ICollisionWorld.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ステージ
		 * @note Add〜 が返す参照は、次に同じ種類を Add するまでの間だけ有効（vector に入れているため）
		 */
		class Stage : public physics::ICollisionWorld
		{
		public:
			// ---- 定義 ----
			/** @brief 名前を設定 */
			inline void SetName(const std::string& name) { m_name = name; }
			inline const std::string& GetName() const { return m_name; }
			/** @brief 地形ブロックを追加（x, y は左下） */
			StageBlock& AddBlock(const char* name, float x, float y, float width, float height);
			/** @brief 光源を追加 */
			LightSource& AddLightSource(const char* name, float x, float y, float angleDeg);
			/** @brief 反射鏡を追加（x, y は中心） */
			LightMirror& AddLightMirror(const char* name, float x, float y, float angleDeg, float length);
			/** @brief 受光器を追加（x, y は左下） */
			LightReceiver& AddLightReceiver(const char* name, float x, float y, float width, float height);
			/** @brief 扉を追加（x, y は左下。requiredReceivers が全部点灯すると開く） */
			Door& AddDoor(const char* name, float x, float y, float width, float height, const std::vector<std::string>& requiredReceivers);
			/** @brief 光の足場を追加（x, y は左下） */
			LightPlatform& AddLightPlatform(const char* name, float x, float y, float width, float height);
			/** @brief 床スイッチを追加（x, y は左下。台の上面に薄く置く） */
			FloorSwitch& AddFloorSwitch(const char* name, float x, float y, float width, float height);
			/** @brief 霧の発生装置を追加（x, y, width, height は霧が広がる範囲。上端が天井。triggerSignal がONで吹き出す） */
			MistEmitter& AddMistEmitter(const char* name, float x, float y, float width, float height, const char* triggerSignal);
			/** @brief 鏡の足場を追加（x, y は左下。霧に触れるまで見えない） */
			MirrorPlatform& AddMirrorPlatform(const char* name, float x, float y, float width, float height);
			/** @brief チェックポイントを追加（x, y は左下。下端の真ん中から再開する） */
			Checkpoint& AddCheckpoint(const char* name, float x, float y, float width, float height);
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
			/** @brief 受光器が点灯しているか（名前で探す。見つからなければ false） */
			bool IsReceiverLit(const std::string& name) const;
			/** @brief 名前で受光器を探す（見つからなければ -1） */
			int FindReceiverIndex(const std::string& name) const;
			/** @brief スイッチが押されているか（名前で探す。見つからなければ false） */
			bool IsSwitchPressed(const std::string& name) const;
			/** @brief 信号がONか（同じ名前の受光器が点灯している、またはスイッチが押されている） */
			bool IsSignalOn(const std::string& name) const;
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

			// エディタから直接編集できるよう、非 const 版も用意している
			inline std::vector<StageBlock>& GetBlocks() { return m_blocks; }
			inline const std::vector<StageBlock>& GetBlocks() const { return m_blocks; }
			inline std::vector<LightSource>& GetLightSources() { return m_lightSources; }
			inline const std::vector<LightSource>& GetLightSources() const { return m_lightSources; }
			inline std::vector<LightMirror>& GetLightMirrors() { return m_lightMirrors; }
			inline const std::vector<LightMirror>& GetLightMirrors() const { return m_lightMirrors; }
			inline std::vector<LightReceiver>& GetLightReceivers() { return m_lightReceivers; }
			inline const std::vector<LightReceiver>& GetLightReceivers() const { return m_lightReceivers; }
			inline std::vector<Door>& GetDoors() { return m_doors; }
			inline const std::vector<Door>& GetDoors() const { return m_doors; }
			inline std::vector<LightPlatform>& GetLightPlatforms() { return m_lightPlatforms; }
			inline const std::vector<LightPlatform>& GetLightPlatforms() const { return m_lightPlatforms; }
			inline std::vector<FloorSwitch>& GetFloorSwitches() { return m_floorSwitches; }
			inline const std::vector<FloorSwitch>& GetFloorSwitches() const { return m_floorSwitches; }
			inline std::vector<MistEmitter>& GetMistEmitters() { return m_mistEmitters; }
			inline const std::vector<MistEmitter>& GetMistEmitters() const { return m_mistEmitters; }
			inline std::vector<MirrorPlatform>& GetMirrorPlatforms() { return m_mirrorPlatforms; }
			inline const std::vector<MirrorPlatform>& GetMirrorPlatforms() const { return m_mirrorPlatforms; }
			inline std::vector<Checkpoint>& GetCheckpoints() { return m_checkpoints; }
			inline const std::vector<Checkpoint>& GetCheckpoints() const { return m_checkpoints; }
			/** @brief 体が触れているチェックポイント（なければ nullptr） */
			const Checkpoint* FindCheckpoint(const math::Rect& body) const;


		public:
			// ---- 当たり判定（ICollisionWorld）----
			void QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const override;
			bool AnyOverlap(const math::Rect& box) const override;
			/**
			 * @brief 地形だけ（ブロック + 閉じた扉 + 鏡の足場）と重なるか
			 * @details 反射鏡を動かせるかの判定に使う（光の足場は含めない）
			 */
			bool OverlapsTerrain(const math::Rect& box) const;


		public:
			Stage() = default;
			~Stage() override = default;


		private:
			/** 光の足場の更新 */
			void UpdateLightPlatforms(float deltaTime);
			/** 扉の更新 */
			void UpdateDoors(float deltaTime);
			/** スイッチの更新 */
			void UpdateFloorSwitches(const std::vector<math::Rect>& bodies);
			/** 霧の更新 */
			void UpdateMistEmitters(float deltaTime);
			/** 鏡の足場の更新（霧に触れたら見えるようにする） */
			void UpdateMirrorPlatforms(float deltaTime);


		private:
			/** ステージ名 */
			std::string m_name;
			std::vector<StageBlock> m_blocks;
			std::vector<LightSource> m_lightSources;
			std::vector<LightMirror> m_lightMirrors;
			std::vector<LightReceiver> m_lightReceivers;
			std::vector<Door> m_doors;
			std::vector<LightPlatform> m_lightPlatforms;
			std::vector<FloorSwitch> m_floorSwitches;
			std::vector<MistEmitter> m_mistEmitters;
			std::vector<MirrorPlatform> m_mirrorPlatforms;
			std::vector<Checkpoint> m_checkpoints;

			/** 奥の鏡 */
			BackMirror m_backMirror;

			/** 光の計算 */
			LightTracer m_lightTracer;
			/** 光の計算結果 */
			LightTraceResult m_lightResult;
		};
	}
}
