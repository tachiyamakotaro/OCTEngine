/**
 * @file FakeWallRenderer.h
 * @brief 偽物の壁（FakeWall）と、手鏡の視界の扇形を描く
 * @details ・偽物の壁は、本物の地形ブロックと同じモデル・同じ置き方で描く（見分けがつかないように）。
 *          ・反射した光で壁ごと消えている壁（FakeWall::IsWhollyRevealed）は描かない。
 *            光で開いた穴（FakeWall::lightHoles）はシェーダーに渡して、その壁の上だけピクセルを捨てる。
 *            DebugScene が StageRenderer の設定を Config に写して渡す。
 *          ・シェーダー（Assets/shader/fakeWall.fx）が、消えている範囲（今の視界と、少し前に当てていた所）の中のピクセルを捨てる（clip）ので、
 *            その部分だけ壁が消えて、向こう側が見える。時間がたつと、ざらざらと元に戻る（ディゾルブ）。
 *          ・fakeWall.fx は model.fx を include しているので、ライティングと影はほかのモデルと同じ。
 *            ライトの値（SceneLight）の後ろに手鏡の値をつなげて、同じ b1 で渡している。
 *          ・手鏡を構えている間は、視界の扇形を半透明で描く（ほかのモデルを全部 Draw したあとに DrawFan を呼ぶ）。
 *          ・光源から出る光は全部、ここで半透明に少し光らせて描く（光の中にいるプレイヤーが透けて見えるように）。
 *            太さは光源の isWideBeam で決まる（細い光は thinBeamWidth）。DrawFan で一緒に登録する。
 *            鏡で反射する所は、光の端を鏡の面に沿って切る（光が鏡からはみ出さないように）。
 *            そのため光は1本ずつ、自分の端の位置を持った定数バッファ（BeamModel::constants）を使う。
 *
 *          エンジンの ModelRender は model.fx 固定なので、MirrorGlass と同じく Model を直接持っている。
 */
#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include "Source/Core/Math/Rect.h"
#include "Source/Stage/Gimmick/HandMirror.h"


namespace app
{
	namespace stage
	{
		class Stage;
	}


	namespace render
	{
		/**
		 * @brief 偽物の壁と手鏡の視界を描く
		 */
		class FakeWallRenderer
		{
		public:
			/**
			 * @brief 設定
			 */
			struct Config
			{
				// ---- 偽物の壁（StageRenderer の地形と同じ値にする）----
				/** 壁のモデル */
				std::string wallModelPath = "Assets/modelData/Box.tkm";
				/** モデルの上方向 */
				EnModelUpAxis wallUpAxis = enModelUpAxisZ;
				/** モデルの元の大きさ */
				Vector3 wallModelSize = Vector3(100.0f, 100.0f, 100.0f);
				/** 箱の原点が底面の中心か（false なら箱の中心） */
				bool isWallPivotAtBottom = true;
				/** 壁の奥行き */
				float wallDepth = 300.0f;
				/** 奥の鏡に映った壁も描くか */
				bool isDrawReflection = true;
				/** 視界の縁のすぐ外側を光らせる幅（ワールド単位。0 で光らせない） */
				float edgeGlowWidth = 10.0f;
				/** 縁の光の色（1 より大きいとブルームで光る） */
				float edgeGlowColor[3] = { 0.9f, 0.5f, 1.8f };
				/** 元に戻るときの、ざらざらの粒の大きさ（ワールド単位） */
				float dissolveCellSize = 6.0f;
				/** 元に戻りかけの所を光らせる幅（消えている度合いで。0 で光らせない） */
				float dissolveGlowWidth = 0.15f;

				// ---- 手鏡の視界の扇形 ----
				/** 扇形を描くか */
				bool isShowFan = true;
				/** 扇形を描く板のモデル（1辺100の箱・原点は中心・Y-up） */
				std::string fanModelPath = "Assets/modelData/gimmick/mirror_glass.tkm";
				/** 板のモデルの1辺の長さ */
				float fanModelSize = 100.0f;
				/** 扇形を置く奥行き（地形の手前の面 = -wallDepth/2 より少し手前） */
				float fanZ = -160.0f;
				/** 扇形の中の色（rgb）と不透明度（a） */
				float fanColor[4] = { 0.55f, 0.62f, 1.0f, 0.12f };
				/** 扇形の縁の線の色（rgb）と不透明度（a） */
				float fanEdgeColor[4] = { 0.75f, 0.82f, 1.0f, 0.75f };
				/** 縁の線の太さ（ワールド単位） */
				float fanEdgeWidth = 3.0f;
				/** 縁の点線の間隔（ワールド単位。0 で実線） */
				float fanDashLength = 24.0f;

				// ---- 光（半透明）----
				/** 細い光（光源の isWideBeam が false）の太さ */
				float thinBeamWidth = 8.0f;
				/** 光を描く箱のモデル（1辺100の箱・原点は中心・Y-up） */
				std::string beamModelPath = "Assets/modelData/gimmick/mirror_glass.tkm";
				/** 箱のモデルの1辺の長さ */
				float beamModelSize = 100.0f;
				/** 光の色（rgb）と不透明度（a） */
				float beamColor[4] = { 1.0f, 0.95f, 0.6f, 0.45f };
				/** こちらを向いている面の不透明度の割合（小さいほど真ん中が透ける。光の上下の縁は a のまま） */
				float beamFrontOpacity = 0.5f;
				/** 光の上下の縁の明るさ（1 より大きいとブルームで少し光る） */
				float beamEdgeGlow = 1.8f;
			};


		public:
			/** @brief 初期化（扇形のモデルを読む。壁のモデルは必要になったときに読む） */
			void Init(const Config& config);
			/** @brief ステージに合わせて壁を置き、手鏡の視界をシェーダーに渡す値にする（毎フレーム、手鏡の操作のあとに呼ぶ） */
			void Update(const stage::Stage& stage);
			/** @brief 偽物の壁を描く（不透明。StageRenderer と一緒に呼ぶ） */
			void DrawWalls(RenderContext& rc);
			/** @brief 光と、手鏡の視界の扇形を描く（半透明。ほかのモデルを全部 Draw したあとに呼ぶ） */
			void DrawFan(RenderContext& rc);
			/** @brief 設定（ImGui での調整用） */
			inline Config& GetConfig() { return m_config; }


		public:
			FakeWallRenderer();
			// Model を持っているのでコピー禁止
			FakeWallRenderer(const FakeWallRenderer&) = delete;
			FakeWallRenderer& operator=(const FakeWallRenderer&) = delete;


		private:
			/** シェーダーに渡せる「消えている範囲」の数（fakeWall.fx の REVEAL_MAX と同じ。HandMirror::MAX_REVEAL_COUNT とも同じにする） */
			static constexpr int REVEAL_MAX = 32;
			/** シェーダーに渡せる「光で開いた穴」の数（全部の偽物の壁の合計。fakeWall.fx の LIGHT_HOLE_MAX と同じ） */
			static constexpr int LIGHT_HOLE_MAX = 16;
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す値（fakeWall.fx の DirectionLightCb と同じ並び）
			 * @details 前半は model.fx と同じライトの値。その後ろに手鏡の値を float4 でつなげる。
			 *          ライトの構造体の大きさは 16 の倍数なので、後ろの float4 は HLSL と同じ位置から始まる。
			 */
			struct Constants
			{
				SceneLightData light;
				float cone[4];			// xy = 扇形の頂点, zw = 向き（長さ1）
				float coneParam[4];		// x = sin(開きの半分), y = cos(開きの半分), z = 長さ, w = 1 なら手鏡を構えている
				float glow[4];			// rgb = 壁の縁の光の色, a = 縁の光の幅
				float fanColor[4];		// rgb = 扇形の色, a = 不透明度
				float fanEdgeColor[4];	// rgb = 扇形の縁の色, a = 不透明度
				float fanParam[4];		// x = 縁の線の太さ, y = 点線の間隔
				float revealCone[REVEAL_MAX][4];	// 消えている範囲：xy = 扇形の頂点, zw = 向き（長さ1）
				float revealParam[REVEAL_MAX][4];	// 消えている範囲：x = sin(開きの半分), y = cos(開きの半分), z = 長さ, w = 消えている度合い（1〜0）
				float revealInfo[4];	// x = 消えている範囲の数, y = ざらざらの粒の大きさ, z = 戻りかけの所を光らせる幅
				float lightHoleSeg[LIGHT_HOLE_MAX][4];		// 光で開いた穴：xy = 線分の始点, zw = 終点（同じなら丸）
				float lightHoleParam[LIGHT_HOLE_MAX][4];	// 光で開いた穴：x = 半径
				float lightHoleClip[LIGHT_HOLE_MAX][4];		// 光で開いた穴：その穴を開けた壁の範囲（xy = 左下, zw = 右上）。ほかの壁は消さない
				float lightHoleInfo[4];		// x = 光で開いた穴の数
				float beamColor[4];			// 太い光：rgb = 色, a = 不透明度
				float beamParam[4];			// 太い光：x = こちらを向いている面の不透明度の割合, y = 縁の明るさ
				float beamStartCap[4];		// 光の始まりの切り口：xy = 面の向き（こちら側を残す）, zw = 面の上の点。xy が 0 なら切らない
				float beamEndCap[4];		// 光の終わりの切り口（同じ形）
			};
			/**
			 * @brief 光1本ぶんのモデルと、そのモデル専用のシェーダーに渡す値
			 * @details 光ごとに切り口の位置が違うので、壁や扇形と共有の m_constants ではなく、1本ずつ持つ
			 */
			struct BeamModel
			{
				Model model;
				Constants constants;
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");


		private:
			/** @brief モデルを読む */
			void InitModel(Model& model, const char* tkmFilePath, EnModelUpAxis upAxis, const char* psEntryPoint, D3D12_CULL_MODE cullMode, bool isTranslucent, Constants* constants = nullptr);
			/** @brief 壁のモデルを1つ取り出す（足りなければ作る） */
			Model& AcquireWall(std::vector<std::unique_ptr<Model>>& pool, size_t& usedCount, D3D12_CULL_MODE cullMode);
			/** @brief 光のモデルを1つ取り出す（足りなければ作る） */
			BeamModel& AcquireBeam();
			/** @brief 壁を矩形に合わせて置く（StageRenderer::Place と同じ置き方） */
			void PlaceWall(Model& model, const math::Rect& rect, bool isReflected, float planeZ);


		private:
			/** 設定 */
			Config m_config;
			/** シェーダーに渡す値（壁と扇形のモデルで共有する） */
			Constants m_constants;
			/** 壁のモデル */
			std::vector<std::unique_ptr<Model>> m_walls;
			size_t m_wallCount;
			/** 鏡に映った壁のモデル（表裏が逆になるので表側を消す設定） */
			std::vector<std::unique_ptr<Model>> m_reflectedWalls;
			size_t m_reflectedWallCount;
			/** 扇形のモデル */
			Model m_fan;
			/** 光のモデル */
			std::vector<std::unique_ptr<BeamModel>> m_beams;
			size_t m_beamCount;
			/** 消えている範囲（毎フレーム使い回す入れ物） */
			std::vector<stage::HandMirrorReveal> m_reveals;
			/** 今フレーム扇形を描くか */
			bool m_isFanDrawn;
			/** 初期化済みか */
			bool m_isInited;
		};
	}
}
