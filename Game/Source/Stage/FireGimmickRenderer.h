/**
 * @file FireGimmickRenderer.h
 * @brief 虫眼鏡のギミック（虫眼鏡・草・ロープ・吊るされた板）と、集まる光・焦点・炎を描く
 * @details ・ふつうのモデル（ModelRender）… 虫眼鏡・草・ロープ・吊るされた板
 *            使うモデルは Tools/make_magnifier_models.py で作った仮モデル（Y-up・原点は中心・だいたい1辺100）。
 *          ・半透明のモデル（光と同じ Assets/shader/lightBeam.fx）… 集まる光の円すい・焦点の光の玉・炎
 *            1つずつ色やゆらぎが違うので、光（LightBeamRenderer）と同じく、1つずつ自分の定数バッファを持つ。
 *          ・虫眼鏡に光が当たっていない間も、うすい円すいと小さな玉で「ここに光が集まる」ことを見せる（isShowGuide）。
 *          ・奥の鏡に映った像は描かない（今は全ステージで奥の鏡を使っていないため）。
 *          ・煙と SE は DebugScene::UpdateFireEffects が出す。
 *          GimmickRenderer に足さずに別のクラスにしたのは、ほかのギミックのブランチとぶつかりにくくするため。
 */
#pragma once
#include "Source/Core/Math/Rect.h"
#include <memory>
#include <string>
#include <type_traits>
#include <vector>


namespace app
{
	namespace stage
	{
		class Stage;
		class Magnifier;
		class BurnableGimmick;


		/**
		 * @brief 虫眼鏡のギミックと、集まる光・炎を描く
		 */
		class FireGimmickRenderer
		{
		public:
			/**
			 * @brief ふつうのモデルの種類
			 */
			enum class ModelKind : int
			{
				Magnifier,		//!< 虫眼鏡（光が当たっていない）
				MagnifierLit,	//!< 虫眼鏡（光が当たっている。ガラスが温かい色）
				Grass,			//!< 草むら
				GrassBurning,	//!< 燃えている草むら（こげた色）
				Rope,			//!< ロープ
				RopeBurning,	//!< 燃えているロープ
				Plank,			//!< 吊るされた板
				Num
			};


			/**
			 * @brief 半透明のモデルの種類
			 */
			enum class GlowKind : int
			{
				Cone,	//!< 集まる光の円すい（-X が底、+X が先）
				Spot,	//!< 焦点の光の玉
				Flame,	//!< 炎（しずくの形。上がとがっている）
				Num
			};


			/**
			 * @brief 色（rgb）と不透明度（a）と、シェーダーの見え方
			 */
			struct GlowStyle
			{
				float color[4];
				/** こちらを向いている面の不透明度の割合（小さいほど真ん中が透ける） */
				float frontOpacity;
				/** 縁の明るさ（1 より大きいとブルームで光る） */
				float edgeGlow;
			};


			/**
			 * @brief 設定
			 */
			struct Config
			{
				/** ふつうのモデルのパス（ModelKind の順） */
				std::string modelPaths[static_cast<int>(ModelKind::Num)] = {
					"Assets/modelData/gimmick/magnifier.tkm",
					"Assets/modelData/gimmick/magnifier_lit.tkm",
					"Assets/modelData/gimmick/grass.tkm",
					"Assets/modelData/gimmick/grass_burning.tkm",
					"Assets/modelData/gimmick/rope.tkm",
					"Assets/modelData/gimmick/rope_burning.tkm",
					"Assets/modelData/gimmick/wood_plank.tkm",
				};
				/** 半透明のモデルのパス（GlowKind の順） */
				std::string glowModelPaths[static_cast<int>(GlowKind::Num)] = {
					"Assets/modelData/gimmick/focus_cone.tkm",
					"Assets/modelData/gimmick/focus_spot.tkm",
					"Assets/modelData/gimmick/flame.tkm",
				};
				/** モデルの元の大きさ */
				float modelSize = 100.0f;

				// ---- 奥行き（Z方向の厚み）----
				float grassDepth = 160.0f;
				float ropeDepth = 12.0f;
				float plankDepth = 220.0f;

				// ---- 集まる光 ----
				/** 円すいの底の太さ（レンズの直径に対する割合。ガラスの大きさに合わせる） */
				float coneWidthRate = 0.9f;
				/** 光が当たっているときの円すい */
				GlowStyle litCone = { { 1.0f, 0.85f, 0.45f, 0.5f }, 0.35f, 2.2f };
				/** 光が当たっていないときの目印の円すい（不透明度はゆっくり明るくなったり暗くなったりする） */
				GlowStyle guideCone = { { 1.0f, 0.95f, 0.75f, 0.12f }, 0.3f, 1.0f };
				/** 目印の円すいの明るさのゆれ（a に足す幅）と速さ */
				float guidePulseAmount = 0.06f;
				float guidePulseSpeed = 2.5f;
				/** 燃やせる焦点の光の玉 */
				GlowStyle hotSpot = { { 1.0f, 0.75f, 0.35f, 0.9f }, 1.0f, 3.0f };
				float hotSpotSize = 24.0f;
				/** さえぎられてぼやけた光（焦点が手前の物の上に広がる） */
				GlowStyle blurSpot = { { 1.0f, 0.9f, 0.6f, 0.35f }, 0.6f, 1.5f };
				float blurSpotSize = 45.0f;
				/** 目印の小さな玉 */
				GlowStyle guideSpot = { { 1.0f, 0.95f, 0.75f, 0.3f }, 0.8f, 1.2f };
				float guideSpotSize = 10.0f;

				// ---- 炎 ----
				/** 外側の炎 */
				GlowStyle flameOuter = { { 1.0f, 0.42f, 0.08f, 0.75f }, 0.55f, 2.8f };
				/** 内側の明るい炎（外側の炎の innerFlameRate の大きさ） */
				GlowStyle flameInner = { { 1.0f, 0.85f, 0.35f, 0.85f }, 0.8f, 3.0f };
				float innerFlameRate = 0.55f;
				/** 草の炎の大きさ（高さ）と、ロープの炎の大きさ */
				float grassFlameSize = 70.0f;
				float ropeFlameSize = 34.0f;
				/** 温まっている間（火がつく前）にくすぶる炎の大きさ（heat が 1 のとき） */
				float smolderFlameSize = 28.0f;
				/** くすぶる炎を出し始める温まり具合 */
				float smolderStartHeat = 0.3f;
				/** 1つの物に出す炎の数の上限 / 全部の炎の数の上限 */
				int maxFlamesPerObject = 24;
				int maxFlames = 120;
			};


		public:
			/** @brief 初期化（モデルは必要になったときに読み込む） */
			void Init(const Config& config);
			/** @brief ステージに合わせてモデルを置く（毎フレーム、ステージの更新のあとに呼ぶ） */
			void Update(const Stage& stage, float deltaTime);
			/** @brief ふつうのモデルを描く（ほかのモデルと一緒に呼ぶ） */
			void DrawOpaque(RenderContext& rc);
			/** @brief 半透明のモデルを描く（光の LightBeamRenderer::Draw のあとに呼ぶ） */
			void DrawTranslucent(RenderContext& rc);
			/** @brief 設定（ImGui での調整用） */
			inline Config& GetConfig() { return m_config; }


		public:
			FireGimmickRenderer() = default;
			// ModelRender・Model を持っているのでコピー禁止
			FireGimmickRenderer(const FireGimmickRenderer&) = delete;
			FireGimmickRenderer& operator=(const FireGimmickRenderer&) = delete;


		private:
			/** SceneLight のライトの構造体（LightBeamRenderer と同じ取り方） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す値（lightBeam.fx の DirectionLightCb と同じ並び。LightBeamRenderer::Constants と同じ）
			 */
			struct Constants
			{
				SceneLightData light;
				float color[4];
				float param[4];
				float startCap[4];
				float endCap[4];
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");

			/** 半透明のモデル1つぶん（自分専用の定数バッファを持つ） */
			struct GlowModel
			{
				Model model;
				Constants constants;
			};

			/** 同じモデルを何個でも置けるようにする入れ物（ふつうのモデル） */
			struct ModelPool
			{
				std::string path;
				std::vector<std::unique_ptr<ModelRender>> models;
				size_t usedCount = 0;
				ModelRender& Acquire();
			};

			/** 同じモデルを何個でも置けるようにする入れ物（半透明のモデル） */
			struct GlowPool
			{
				std::string path;
				std::vector<std::unique_ptr<GlowModel>> models;
				size_t usedCount = 0;
				GlowModel& Acquire();
			};


		private:
			/** ふつうのモデルを1つ置く（angleDeg は Z 軸まわり） */
			void Put(ModelKind kind, const math::Vec2& center, float z, float angleDeg, const Vector3& size);
			/** 矩形にぴったり合うように置く */
			void PutRect(ModelKind kind, const math::Rect& rect, float depth);
			/**
			 * @brief 半透明のモデルを1つ置く
			 * @param endCapNormal / endCapPoint この面より向こう（endCapNormal と逆の側）を捨てる。向きが 0 なら切らない
			 */
			void PutGlow(GlowKind kind, const GlowStyle& style, const Vector3& position, float angleDeg, const Vector3& size,
				const math::Vec2& endCapNormal = math::Vec2::Zero(), const math::Vec2& endCapPoint = math::Vec2::Zero());
			/** 虫眼鏡と、集まる光・焦点 */
			void PutMagnifier(const Magnifier& magnifier);
			/** 燃えている物の炎（くすぶっている物の小さな炎も） */
			void PutFlames(const BurnableGimmick& burnable, bool isRope, float frontZ, int objectIndex);
			/** 炎を1つ置く（position は炎の根元） */
			void PutFlame(const math::Vec2& position, float z, float size, float phase);


		private:
			/** 設定 */
			Config m_config;
			/** ふつうのモデル */
			ModelPool m_pools[static_cast<int>(ModelKind::Num)];
			/** 半透明のモデル */
			GlowPool m_glowPools[static_cast<int>(GlowKind::Num)];
			/** 炎のゆらぎや目印の明るさに使う時間 */
			float m_time = 0.0f;
			/** 今フレーム置いた炎の数 */
			int m_flameCount = 0;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
