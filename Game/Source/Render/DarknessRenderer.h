/**
 * @file DarknessRenderer.h
 * @brief 暗がり（DarkZone）を、黒い半透明の板で描く。光の近く・プレイヤーのまわり・見えた足場の上だけ暗さを抜く
 * @details ・暗がり1つにつき、手前（planeZ）に黒い半透明の板を1枚置く（シェーダーは Assets/shader/darkness.fx）。
 *            ステージ全体の暗がり（DarkZone::isWholeStage）なら、プレイヤーを真ん中にした、画面より十分大きな板にする。
 *          ・明るい所と暗い所の境目は、ぼかしの幅で少しずつ暗くなる（くっきり分かれない）。
 *            いくつもの明るい所が重なる所は、明るさを「1 −（1 − 明るさ）の積」で合わせる（重なりの所に筋が出ないように）。
 *            板は地形の手前の面より手前に置くので、地形も足場もまとめて暗くなる。
 *          ・ピクセルごとに「光の線分・プレイヤー・見えた暗がりの足場（HiddenFloor）」までの距離を調べて、近いほど透明にする。
		 *            プレイヤーのまわりは、マリオの暗闇ステージのように、いつもはっきり見える丸にする。
 *            板は手前にあるので、カメラから見て z=0（ギミックやプレイヤーがいる面）のどこに重なるかに直してから距離を測る。
 *          ・暗がりの左右の端は斜めにでき（DarkZone::slantLeft / slantRight）、途中で折り曲げられる（bendLeft / bendRight と、その高さ）。板は斜めの分も入る大きさにして、
 *            シェーダーで斜めの端の外側を透明にする（端は edgeFeather で、端に直角な向きにぼかす）。
 *          ・手前の壁（FrontWall）も、同じシェーダーの別の入り口（PSMainFrontWall）で描く。
 *            壁は光や半透明の物よりも手前に見せたいので、DrawFrontWalls で一番最後に登録する。
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
				/** 見えた足場のまわりを、足場からどれだけ広く明るくするか（この外側を、さらに rectSoftWidth でぼかす） */
				float rectMargin = 5.0f;
				/** 見えた足場のまわりの明るい所の、外側のぼかしの幅（小さいほど、足場のまわりの明るい所が小さい） */
				float rectSoftWidth = 35.0f;
				/** ステージ全体の暗がりの板の大きさ（プレイヤーを真ん中に、この幅と高さで置く。画面より十分大きくする） */
				float wholeStageSize = 40000.0f;
				/** 手前の壁（FrontWall）の板を置く奥行き（暗がりの板より手前にする） */
				float frontWallZ = -180.0f;
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
			/** @brief 手前の壁（FrontWall）を描く（半透明。光や炎など、ほかの半透明の物を全部登録したあとに呼ぶ） */
			void DrawFrontWalls(RenderContext& rc);
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
				float param[4];						// x = 暗さ, y = 光の半径（境目の真ん中）, z = 光の境目のぼかしの幅, w = 範囲の端のぼかし
				float view[4];						// xyz = カメラの前の向き, w = 平行投影なら 1
				float player[4];					// xy = プレイヤーの体の中心, z = 線分の数, w = 足場の数
				float zone[4];						// xy = 暗がりの左下, zw = 右上
				float color[4];						// rgb = 暗がりの色, a = 見えた足場のまわりのぼかしの幅（rectSoftWidth）
				float playerLight[4];				// x = プレイヤーのまわりの丸の半径（境目の真ん中）, y = 丸の境目のぼかしの幅, z = ステージ全体なら 1
				float slant[4];						// x = 左の端の傾き（上の辺でのずれ）, y = 右の端の傾き, z = 左の端の折れ目のずれ, w = 右の端の折れ目のずれ
				float bend[4];						// x = 左の端の折れ目の高さ（0〜1）, y = 右の端の折れ目の高さ, zw = 使わない
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
			/** @brief 手前の壁のモデルを1つ取り出す（足りなければ作る） */
			ZoneModel& AcquireWall();
			/** @brief 板のモデルを作る（psEntry はピクセルシェーダーの入り口） */
			std::unique_ptr<ZoneModel> CreateModel(const char* psEntry);


		private:
			/** 設定 */
			Config m_config;
			/** 暗がりのモデル（unique_ptr なので、増えても constants の場所は変わらない。Model はこの場所を覚えている） */
			std::vector<std::unique_ptr<ZoneModel>> m_zones;
			/** 手前の壁のモデル */
			std::vector<std::unique_ptr<ZoneModel>> m_walls;
			/** 今フレーム使っている手前の壁の数 */
			size_t m_wallCount;
			/** 今フレーム使っている数 */
			size_t m_zoneCount;
			/** 初期化済みか */
			bool m_isInited;
		};
	}
}
