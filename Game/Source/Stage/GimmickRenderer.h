/**
 * @file GimmickRenderer.h
 * @brief ステージのギミック（扉・受光器・光の足場・光源・反射鏡・光・奥の鏡の枠・床スイッチ・霧の吹き出し口・鏡の足場・ランプ・ワープ・ゴール・氷・日よけ板・日光の窓）をモデルで描く
 * @details ギミックの位置・大きさ・状態（扉の開き具合、受光器の点灯など）は毎フレーム Stage から読むので、
 *          エディタで動かしたり、遊んでいる最中に状態が変わったりしても、そのまま追従する。
 *          奥の鏡があるときは、鏡に映った像も一緒に描く。
 *
 *          使うモデルはどれも「1辺100の箱・原点は中心」（Tools/make_gimmick_models.py で作った仮モデル）。
 *          本番のモデルに差し替えるときは、同じ大きさ・原点・向きで作って Config のパスを変えるだけでよい。
 *            ・反射鏡 … 表（鏡面）を +Y に向ける。コードが Z 軸まわりに鏡の角度だけ回す
 *            ・光源   … 光の出る面を +X に向ける。コードが Z 軸まわりに光の角度だけ回す
 *            ・光     … +X 方向に伸びる棒。コードが線分の長さに伸ばして回す
 *            ・霧の吹き出し口 … 吹き出す面を -Y（下）に向ける
 *          鏡の足場は「見えるようになってから」だけ描く（見えないうちは風景を映しているので何も描かない）。
 *          霧と露のエフェクトはここでは描かない（Game::UpdateStageEvents で出す）。
 */
#pragma once
#include "Source/Core/Math/Rect.h"
#include <memory>
#include <string>
#include <vector>


namespace app
{
	namespace stage
	{
		class Stage;


		/**
		 * @brief ギミックをモデルで描く
		 */
		class GimmickRenderer
		{
		public:
			/**
			 * @brief モデルの種類
			 */
			enum class ModelKind : int
			{
				Door,			//!< 扉
				ReceiverOff,	//!< 受光器（消灯）
				ReceiverOn,		//!< 受光器（点灯）
				LightPlatform,	//!< 光の足場（出ているときだけ描く）
				LightSource,	//!< 光源
				LightMirror,	//!< 反射鏡
				LightBeam,		//!< 光
				MirrorFrame,	//!< 奥の鏡の枠
				FloorSwitchOff,	//!< 床スイッチ（押されていない）
				FloorSwitchOn,	//!< 床スイッチ（押された。低くつぶして描く）
				MistVent,		//!< 霧の吹き出し口（天井に並べる）
				MirrorPlatform,	//!< 鏡の足場（見えるようになってから描く）
				IndicatorOff,	//!< 合図のランプ（消灯）
				IndicatorOn,	//!< 合図のランプ（点灯）
				WarpGate,		//!< ワープの暗がり（通路の奥。入ると暗がりに消えるように見せる）
				Goal,			//!< ゴール
				IceMelt,		//!< 溶かす氷（白っぽい水色・ひび）
				IceMeltLit,		//!< 溶かす氷（光が当たって溶けている最中。オレンジがかる）
				IceKeep,		//!< 守る氷（濃い青・白い縁）
				IceKeepLit,		//!< 守る氷（光が当たって溶けている最中。縁がオレンジになる）
				ShadeBoard,		//!< 日よけ板（木の板）
				SunWindow,		//!< 日光の窓（光が差し込む天窓の枠）
				Num
			};


			/**
			 * @brief 設定
			 */
			struct Config
			{
				/** モデルのパス（ModelKind の順） */
				std::string modelPaths[static_cast<int>(ModelKind::Num)] = {
					"Assets/modelData/gimmick/door.tkm",
					"Assets/modelData/gimmick/receiver_off.tkm",
					"Assets/modelData/gimmick/receiver_on.tkm",
					"Assets/modelData/gimmick/light_platform.tkm",
					"Assets/modelData/gimmick/light_source.tkm",
					"Assets/modelData/gimmick/light_mirror.tkm",
					"Assets/modelData/gimmick/light_beam.tkm",
					"Assets/modelData/gimmick/mirror_frame.tkm",
					"Assets/modelData/gimmick/floor_switch_off.tkm",
					"Assets/modelData/gimmick/floor_switch_on.tkm",
					"Assets/modelData/gimmick/mist_vent.tkm",
					"Assets/modelData/gimmick/mirror_platform.tkm",
					"Assets/modelData/gimmick/indicator_off.tkm",
					"Assets/modelData/gimmick/indicator_on.tkm",
					"Assets/modelData/gimmick/warp_gate.tkm",
					"Assets/modelData/gimmick/goal.tkm",
					"Assets/modelData/gimmick/ice_melt.tkm",
					"Assets/modelData/gimmick/ice_melt_lit.tkm",
					"Assets/modelData/gimmick/ice_keep.tkm",
					"Assets/modelData/gimmick/ice_keep_lit.tkm",
					"Assets/modelData/gimmick/shade_board.tkm",
					"Assets/modelData/gimmick/sun_window.tkm",
				};
				/** モデルの上方向（仮モデルは Y-up） */
				EnModelUpAxis upAxis = enModelUpAxisY;
				/** モデルの元の大きさ（仮モデルは1辺100の箱） */
				float modelSize = 100.0f;

				// ---- 奥行き（Z方向の厚み）----
				float doorDepth = 120.0f;
				float receiverDepth = 60.0f;
				float lightPlatformDepth = 200.0f;
				float lightSourceDepth = 40.0f;
				float lightMirrorDepth = 80.0f;
				float floorSwitchDepth = 70.0f;
				float mistVentDepth = 80.0f;
				float mirrorPlatformDepth = 200.0f;
				float indicatorDepth = 40.0f;
				/** ワープの暗がりの奥行き（床の奥行き 300 と同じにすると、通路の奥がまるごと暗くなる） */
				float warpGateDepth = 300.0f;
				float goalDepth = 150.0f;
				float iceDepth = 200.0f;
				float shadeBoardDepth = 150.0f;
				float sunWindowDepth = 200.0f;
				// ---- 大きさ ----
				/** 光源の箱の一辺 */
				float lightSourceSize = 30.0f;
				/** 反射鏡の板の厚さ */
				float lightMirrorThickness = 10.0f;
				/** 光の太さ（今は使っていない。光は LightBeamRenderer が半透明で描き、細い光の太さはそちらの Config の thinBeamWidth） */
				float lightBeamThickness = 8.0f;
				/** 奥の鏡の枠の太さ */
				float mirrorFrameThickness = 20.0f;
				/** 押されたスイッチの高さの割合（押されると低くなる） */
				float floorSwitchPressedScale = 0.35f;
				/** 霧の吹き出し口の大きさ（幅・高さ） */
				float mistVentWidth = 60.0f;
				float mistVentHeight = 30.0f;
				/** 日光の窓の枠の太さ */
				float sunWindowThickness = 20.0f;

				/** 鏡に映った像も描くか */
				bool isDrawReflection = true;
			};


		public:
			/**
			 * @brief 初期化（モデルは必要になったときに読み込む）
			 */
			void Init(const Config& config);
			/**
			 * @brief ステージに合わせてモデルを置く（毎フレーム、ステージと反射鏡の操作の更新のあとに呼ぶ）
			 */
			void Update(const Stage& stage);
			/**
			 * @brief 描く
			 */
			void Draw(RenderContext& rc);
			/**
			 * @brief 設定を取得（ImGui での調整用）
			 */
			inline Config& GetConfig() { return m_config; }
			/**
			 * @brief 今フレーム置いたモデルの数（本物 + 鏡に映った像）
			 */
			size_t GetPlacedCount() const;


		public:
			GimmickRenderer() = default;
			// ModelRender を持っているのでコピー禁止
			GimmickRenderer(const GimmickRenderer&) = delete;
			GimmickRenderer& operator=(const GimmickRenderer&) = delete;


		private:
			/**
			 * @brief 同じモデルを何個でも置けるようにする入れ物
			 * @details ModelRender の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す
			 */
			struct ModelPool
			{
				std::string path;
				EnModelUpAxis upAxis = enModelUpAxisY;
				D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
				std::vector<std::unique_ptr<ModelRender>> models;
				size_t usedCount = 0;

				/** 1つ取り出す（足りなければ作る） */
				ModelRender& Acquire();
			};


		private:
			/**
			 * @brief モデルを1つ置く（奥の鏡があれば鏡に映った像も置く）
			 * @param kind モデルの種類
			 * @param center 中心（XY平面）
			 * @param z 奥行きの位置
			 * @param angleDeg Z 軸まわりの回転（度）
			 * @param size 大きさ（X:幅 / Y:高さ / Z:奥行き）
			 * @param isReflectable 鏡に映すか
			 */
			void Put(ModelKind kind, const math::Vec2& center, float z, float angleDeg, const Vector3& size, bool isReflectable = true);
			/** 矩形にぴったり合うように置く */
			void PutRect(ModelKind kind, const math::Rect& rect, float depth);
			/** 奥の鏡の枠を置く */
			void PutMirrorFrame(const Stage& stage);
			/** 霧の吹き出し口を天井に並べる */
			void PutMistVents(const Stage& stage);


		private:
			/** 設定 */
			Config m_config;
			/** 本物のモデル */
			ModelPool m_pools[static_cast<int>(ModelKind::Num)];
			/** 鏡に映ったモデル（表裏が逆になるので表側を消す設定） */
			ModelPool m_reflectionPools[static_cast<int>(ModelKind::Num)];
			/** 霧の吹き出し口の位置（毎フレーム使い回す入れ物） */
			std::vector<math::Vec2> m_ventPositions;
			/** 氷の残っているマスをまとめた矩形（毎フレーム使い回す入れ物） */
			std::vector<math::Rect> m_iceRects;
			/** 今フレームの奥の鏡（Update の間だけ使う） */
			bool m_isMirrorEnabled = false;
			float m_mirrorPlaneZ = 0.0f;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
