/**
 * @file DarknessRenderer.h
 * @brief 暗がり（DarkZone）を、黒い半透明の板で描く。光の近く・プレイヤーのまわり・見えた足場の上だけ暗さを抜く
 * @details ・暗がり1つにつき、範囲の手前（planeZ）に黒い半透明の板を1枚置く（シェーダーは Assets/shader/darkness.fx）。
 *            板は地形の手前の面より手前に置くので、地形も足場もまとめて暗くなる。
 *          ・ピクセルごとに「光の線分・プレイヤー・見えた暗がりの足場（HiddenFloor）」までの距離を調べて、近いほど透明にする。
		 *            プレイヤーのまわりは、マリオの暗闇ステージのように、いつもはっきり見える丸にする。
 *            板は手前にあるので、カメラから見て z=0（ギミックやプレイヤーがいる面）のどこに重なるかに直してから距離を測る。
 *          ・光は LightBeamRenderer が板のあとに描くので、暗がりの中でも光の筋ははっきり見える。
 *          ・LightBeamRenderer と同じく、エンジンの ModelRender は model.fx 固定なので Model を直接持っている。
 */
#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include "Source/Core/Math/Vec2.h"


namespace app
{
	namespace stage
	{
		class Stage;
	}


	namespace render
	{
		/**
		 * @brief 暗がりを描く
		 */
		class DarknessRenderer
		{
		public:
			/** シェーダーに渡せる光の線分の数（多すぎる分は使わない。darkness.fx の DARK_MAX_SEGMENTS と同じにする） */
			static constexpr int MAX_SEGMENTS = 48;
			/** シェーダーに渡せる、見えた足場の数（darkness.fx の DARK_MAX_RECTS と同じにする） */
			static constexpr int MAX_RECTS = 16;


			/**
			 * @brief 設定
			 */
			struct Config
			{
				/** 板を描く箱のモデル（1辺100の箱・原点は中心・Y-up） */
				std::string modelPath = "Assets/modelData/gimmick/mirror_glass.tkm";
				/** 箱のモデルの1辺の長さ */
				float modelSize = 100.0f;
				/** 板を置く奥行き（地形の手前の面 -150 より手前にする） */
				float planeZ = -170.0f;
				/** 板の厚さ */
				float thickness = 2.0f;
				/** 暗がりの色（rgb） */
				float color[3] = { 0.0f, 0.0f, 0.02f };
				/** 見えた足場のまわりを、足場からどれだけ広く明るくするか */
				float rectMargin = 30.0f;
				/** 明るい所の端のぼかし（半径に対する割合。0.5 なら半径の半分から外へ向かって暗くなる） */
				float softness = 0.5f;
			};


		public:
			/** @brief 初期化（モデルは必要になったときに読む） */
			void Init(const Config& config);
			/**
			 * @brief 暗がりに合わせて板を置く（毎フレーム、ステージの更新のあとに呼ぶ）
			 * @param stage ステージ
			 * @param playerCenter プレイヤーの体の中心（XY平面）
			 * @param isOrtho カメラが平行投影か（板から z=0 の面に直すときの向きが変わる）
			 */
			void Update(const stage::Stage& stage, const math::Vec2& playerCenter, bool isOrtho);
			/** @brief 描く（半透明。ほかのモデルのあと、光の前に呼ぶ） */
			void Draw(RenderContext& rc);
			/** @brief 設定（ImGui での調整用） */
			inline Config& GetConfig() { return m_config; }
			/** @brief 今フレーム描く暗がりの数 */
			inline size_t GetDrawCount() const { return m_zoneCount; }


		public:
			DarknessRenderer();
			// Model を持っているのでコピー禁止
			DarknessRenderer(const DarknessRenderer&) = delete;
			DarknessRenderer& operator=(const DarknessRenderer&) = delete;


		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す値（darkness.fx の DirectionLightCb の後ろと同じ並び）
			 */
			struct Constants
			{
				SceneLightData light;
				float param[4];						// x = 暗さ, y = 光の半径, z = プレイヤーの光の半径, w = 範囲の端のぼかし
				float view[4];						// xyz = カメラの前の向き, w = 平行投影なら 1
				float player[4];					// xy = プレイヤーの体の中心, z = 線分の数, w = 足場の数
				float zone[4];						// xy = 暗がりの左下, zw = 右上
				float color[4];						// rgb = 暗がりの色, a = 明るい所の端のぼかし（割合）
				float playerLight[4];				// x = プレイヤーのまわりの丸の半径, y = 丸の端のぼかし（割合）
				float segments[MAX_SEGMENTS][4];	// xy = 光の線分の始点, zw = 終点
				float rects[MAX_RECTS][4];			// xy = 見えた足場の左下, zw = 右上（まわりの広さを足した後）
				float rectVisibility[MAX_RECTS][4];	// x = 見え具合（0〜1）
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");

			/**
			 * @brief 暗がり1つぶんのモデルと、そのモデル専用のシェーダーに渡す値
			 */
			struct ZoneModel
			{
				Model model;
				Constants constants;
			};


		private:
			/** @brief 暗がりのモデルを1つ取り出す（足りなければ作る） */
			ZoneModel& AcquireZone();


		private:
			/** 設定 */
			Config m_config;
			/** 暗がりのモデル（unique_ptr なので、増えても constants の場所は変わらない。Model はこの場所を覚えている） */
			std::vector<std::unique_ptr<ZoneModel>> m_zones;
			/** 今フレーム使っている数 */
			size_t m_zoneCount;
			/** 初期化済みか */
			bool m_isInited;
		};
	}
}
