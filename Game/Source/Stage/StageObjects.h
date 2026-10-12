/**
 * @file StageObjects.h
 * @brief ステージに置く物のデータ（ブロック・光源・反射鏡・受光器・扉・光の足場・スイッチ・霧・鏡の足場・チェックポイント）
 * @details どれも「データだけ」の構造体にして、処理は Stage / LightTracer にまとめている。
 *          ImGui エディタから値を直接いじれるよう、メンバは public にしている。
 *          名前（name）はエディタの表示と、つながり（扉 → 受光器/スイッチ、霧 → スイッチ）に使う。
 *          「信号」= 受光器の点灯 または スイッチが押されていること。名前で指定する。
 */
#pragma once
#include <string>
#include <vector>
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ステージの奥の壁にある大きな鏡
		 * @details 手前の部屋とプレイヤーを映す（見た目だけで、当たり判定や光には関係しない）。
		 *          鏡の面は Z = planeZ の平面。手前(-Z)にあるものが、奥(+Z)に反転して映る。
		 *          frame は鏡の枠（XY平面での範囲）。デバッグ表示とエディタで使う。
		 */
		struct BackMirror
		{
			/** 鏡を使うか */
			bool isEnabled = false;
			/** 鏡の面のZ座標（床の奥行きの奥端に合わせる） */
			float planeZ = 150.0f;
			/** 鏡の枠（左下とサイズ） */
			math::Rect frame = math::Rect::FromMinSize(0.0f, 0.0f, 1000.0f, 600.0f);


			/** 奥行き z の点が鏡に映ったときの奥行き */
			float ReflectZ(const float z) const { return 2.0f * planeZ - z; }
		};


		/**
		 * @brief 固い地形ブロック
		 */
		struct StageBlock
		{
			std::string name;
			/** 範囲 */
			math::Rect rect;
		};


		/**
		 * @brief 光源（向いている方向にまっすぐ光を出す）
		 */
		struct LightSource
		{
			std::string name;
			/** 光が出る位置 */
			math::Vec2 position;
			/** 光の向き（度。0=右、90=上） */
			float angleDeg = 0.0f;
			/** 光を出しているか */
			bool isOn = true;
		};


		/**
		 * @brief 光を反射する鏡（線分）
		 * @details 表側（法線の向き）だけが光を反射する。裏側に当たった光は止まる。
		 *          isDoubleSided を true にすると両面で反射する。
		 */
		struct LightMirror
		{
			std::string name;
			/** 鏡の中心 */
			math::Vec2 center;
			/** 鏡面の傾き（度。0=水平で表は上向き、90=垂直で表は左向き） */
			float angleDeg = 45.0f;
			/** 鏡の長さ */
			float length = 80.0f;
			/** 両面で反射するか */
			bool isDoubleSided = false;
			/** プレイヤーが回転できるか */
			bool isRotatable = true;
			/** 1回の回転で回る角度（度） */
			float rotationStepDeg = 45.0f;
			/** プレイヤーが動かせるか */
			bool isMovable = true;
			/** 動かせる範囲を制限するか */
			bool hasRail = false;
			/** 動かせる範囲の左端（中心のX） */
			float railMinX = 0.0f;
			/** 動かせる範囲の右端（中心のX） */
			float railMaxX = 0.0f;


			/** 鏡面に沿った向き */
			math::Vec2 GetDirection() const { return math::DirectionFromDeg(angleDeg); }
			/** 表側の法線 */
			math::Vec2 GetNormal() const
			{
				const math::Vec2 dir = GetDirection();
				return { -dir.y, dir.x };
			}
			/** 端点（始点） */
			math::Vec2 GetStart() const { return center - GetDirection() * (length * 0.5f); }
			/** 端点（終点） */
			math::Vec2 GetEnd() const { return center + GetDirection() * (length * 0.5f); }
			/** 鏡を囲む矩形（当たり判定の確認用） */
			math::Rect GetBounds() const
			{
				const math::Vec2 a = GetStart();
				const math::Vec2 b = GetEnd();
				return { { math::Min(a.x, b.x), math::Min(a.y, b.y) }, { math::Max(a.x, b.x), math::Max(a.y, b.y) } };
			}
			/**
			 * @brief 回転させる
			 * @param steps 回す段数（正で反時計回り）
			 */
			void Rotate(const int steps)
			{
				angleDeg = math::NormalizeDeg(angleDeg + rotationStepDeg * static_cast<float>(steps));
			}
			/**
			 * @brief レールの範囲に収めた中心X
			 */
			float ClampToRail(const float x) const
			{
				return hasRail ? math::Clamp(x, railMinX, railMaxX) : x;
			}
		};


		/**
		 * @brief 受光器（光が当たっている間「点灯」する）
		 */
		struct LightReceiver
		{
			std::string name;
			/** 範囲（光はここで止まる） */
			math::Rect rect;

			/** 【実行時】光が当たっているか */
			bool isLit = false;
		};


		/**
		 * @brief 扉（つながった信号＝受光器・スイッチが全部ONになると開く）
		 * @details 開くと下端が上に上がっていく（天井に引っ込むシャッター）。
		 */
		struct Door
		{
			std::string name;
			/** 閉じているときの範囲 */
			math::Rect closedRect;
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


			/** 今の当たり判定の範囲 */
			math::Rect GetCurrentRect() const
			{
				math::Rect rect = closedRect;
				rect.min.y += closedRect.Height() * openAmount;
				return rect;
			}
			/** 当たり判定があるか（完全に開いたらない） */
			bool IsSolid() const { return openAmount < 1.0f; }
		};


		/**
		 * @brief 光の足場（光が当たっている間だけ乗れる）
		 */
		struct LightPlatform
		{
			std::string name;
			/** 範囲 */
			math::Rect rect;
			/** 光が外れてから消えるまでの猶予（ちらつき防止） */
			float lingerTime = 0.15f;

			/** 【実行時】今フレーム光が当たっているか */
			bool isLit = false;
			/** 【実行時】足場として残っている時間 */
			float solidTimer = 0.0f;


			/** 足場として乗れるか */
			bool IsSolid() const { return solidTimer > 0.0f; }
		};

		/**
		 * @brief 床スイッチ（キャラクターが踏むと押される）
		 * @details 当たり判定はない（台の上に薄く置く飾りの板）。キャラクターの体が板と重なると押される。
		 *          名前を信号として、扉や霧の発生装置から参照する。
		 */
		struct FloorSwitch
		{
			std::string name;
			/** 板の範囲（台の上面に乗せる） */
			math::Rect rect;
			/** 一度踏んだら押されたままにするか */
			bool isLatch = true;

			/** 【実行時】押されているか */
			bool isPressed = false;
		};


		/**
		 * @brief 霧の発生装置（天井から霧を吹き出す）
		 * @details 信号（triggerSignal）がONになると一度だけ吹き出す。
		 *          霧は area の上端（天井）から下へ fallSpeed で広がり、duration 秒吹いたあと fadeTime 秒で晴れる。
		 *          霧が触れた鏡の足場は見えるようになる（露が残るので、霧が晴れても見えたまま）。
		 */
		struct MistEmitter
		{
			std::string name;
			/** 霧が広がる範囲（上端が天井＝吹き出し口の高さ） */
			math::Rect area;
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


			/** 霧が出ているか（吹いている間 + 晴れるまで） */
			bool IsActive() const { return hasFired && timer < duration + fadeTime; }
			/** 霧の先端（いちばん下まで届いている高さ） */
			float GetFrontY() const
			{
				if (!hasFired) { return area.max.y; }
				return math::Max(area.min.y, area.max.y - fallSpeed * timer);
			}
			/** 霧の濃さ（0〜1。吹き始めに濃くなり、吹き終わると薄れて消える） */
			float GetDensity() const
			{
				if (!IsActive()) { return 0.0f; }
				constexpr float FADE_IN_TIME = 0.3f;
				const float fadeIn = math::Clamp01(timer / FADE_IN_TIME);
				const float fadeOut = timer <= duration ? 1.0f : 1.0f - math::Clamp01((timer - duration) / math::Max(fadeTime, 0.001f));
				return fadeIn * fadeOut;
			}
		};


		/**
		 * @brief 鏡の足場（周りの風景を映していて見えない足場）
		 * @details 霧が触れると表面に露がついて見えるようになり、霧が晴れても露が残るので見えたまま。
		 *          isSolidWhenHidden が true なら、見えないうちから乗れる（本当にそこにある鏡）。
		 *          false にすると、見えるようになるまで当たり判定がない。
		 */
		struct MirrorPlatform
		{
			std::string name;
			/** 範囲 */
			math::Rect rect;
			/** 見えないうちから当たり判定があるか */
			bool isSolidWhenHidden = true;

			/** 【実行時】霧に触れて見えるようになったか（一度見えたら見えたまま） */
			bool isRevealed = false;
			/** 【実行時】見えるようになってからの時間（露の演出用） */
			float revealedTime = 0.0f;
			/** 【実行時】このフレームに見えるようになったか（エフェクトや音を出すきっかけに使う） */
			bool isRevealedThisFrame = false;


			/** 当たり判定があるか */
			bool IsSolid() const { return isRevealed || isSolidWhenHidden; }
		};


		/**
		 * @brief チェックポイント（ここに触れると、死んだときにここから再開する）
		 * @details 再開するのは範囲の下端の真ん中（床の上に置くと、その床に立った状態で再開する）。
		 *          ステージの仕掛け（扉・スイッチ・霧など）は再開のときに最初の状態に戻る。
		 */
		struct Checkpoint
		{
			std::string name;
			/** 触れると有効になる範囲 */
			math::Rect rect;


			/** 再開する位置（足元） */
			math::Vec2 GetSpawnPosition() const { return { (rect.min.x + rect.max.x) * 0.5f, rect.min.y }; }
		};


		/**
		 * @brief ワープ（体の中心が入ると、別の場所へ飛ばす）
		 * @details マリオのお城のような「間違った道を進むと最初に戻される」仕掛けや、正解の道からゴールへ運ぶのに使う。
		 *          飛んだあとも走る勢いはそのまま残る。
		 *          disabledBySignal に信号の名前を入れると、その信号がONの間はワープしない（謎を解くと通れる道などに使う）。
		 */
		struct WarpZone
		{
			std::string name;
			/** 範囲（体の中心がここに入るとワープ） */
			math::Rect rect;
			/** 飛ぶ先（足元） */
			math::Vec2 destination;
			/** この信号がONの間はワープしない（空ならいつでもワープする） */
			std::string disabledBySignal;
		};


		/**
		 * @brief 合図のランプ（信号がONになると光る。見た目だけ）
		 * @details 光の謎を解くと正解の道のランプが光る、など。signal が空のランプはずっと消えたまま（ダミー）。
		 */
		struct Indicator
		{
			std::string name;
			/** 範囲 */
			math::Rect rect;
			/** 光るきっかけの信号（受光器・スイッチの名前） */
			std::string signal;

			/** 【実行時】光っているか */
			bool isOn = false;
		};


		/**
		 * @brief ゴール（体が触れるとステージクリア）
		 */
		struct Goal
		{
			std::string name;
			/** 範囲 */
			math::Rect rect;
		};
	}
}
