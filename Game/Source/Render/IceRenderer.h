/**
 * @file IceRenderer.h
 * @brief 氷を、穴の縁がなめらかに丸く消えていくように描く
 * @details ・氷1つを箱のモデル1つで描き、穴の形はシェーダー（Assets/shader/ice.fx）で切り抜く。
 *            氷のマスごとの「残り具合」（IceBlock::BuildSmoothField。1 = 冷たい氷 〜 0 = 溶けた）を
 *            構造化バッファで渡し、シェーダーがとなりのマスとなめらかにつないで、残り具合が threshold より小さい所を捨てる。
 *            → マスの四角い形は見えず、穴の縁は丸い線になる。温まったマスは溶ける前から少しずつ縁が下がっていくので、
 *              溶け方もなめらかに見える。
 *          ・穴の縁は少し明るくする（溶けている最中はオレンジがかった色で光る）。
 *          ・穴の向こうに見える箱の内側（裏の面）は少し暗くして、氷の厚みに見せる（両面を描く）。
 *          ・奥の鏡があるときは、鏡に映った像も描く。
 *          ・光が当たっている間・通れる穴から溶け広がっている間は「溶けている最中」のモデルにする（GimmickRenderer と同じモデル）。
 *          ・ice.fx は model.fx を include しているので、ライトの値（カメラの位置など）は model.fx と同じ並びで渡す。
 *
 *          エンジンの ModelRender は model.fx 固定なので、LightBeamRenderer と同じく Model を直接持っている。
 *          GimmickRenderer が持っていて、GimmickRenderer の Update / Draw から呼ばれる。
 */
#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		class Stage;
		class IceBlock;
	}


	namespace render
	{
		/**
		 * @brief 氷をなめらかに描く
		 */
		class IceRenderer
		{
		public:
			/**
			 * @brief 設定
			 */
			struct Config
			{
				/** モデル（1辺100の箱・原点は中心・Y-up）：溶かす氷・溶かす氷（溶けている最中）・守る氷・守る氷（溶けている最中） */
				std::string meltModelPath = "Assets/modelData/gimmick/ice_melt.tkm";
				std::string meltLitModelPath = "Assets/modelData/gimmick/ice_melt_lit.tkm";
				std::string keepModelPath = "Assets/modelData/gimmick/ice_keep.tkm";
				std::string keepLitModelPath = "Assets/modelData/gimmick/ice_keep_lit.tkm";
				/** モデルの元の大きさ */
				float modelSize = 100.0f;
				/** 氷の奥行き（GimmickRenderer の iceDepth を毎フレーム写す） */
				float depth = 200.0f;
				/** 残り具合がこれより小さい所は描かない（大きいほど、温まった所が早く消えて見える） */
				float threshold = 0.3f;
				/** 縁を明るくする幅（残り具合で。threshold からこの分だけ上まで明るくなる） */
				float edgeWidth = 0.18f;
				/** 穴の向こうに見える内側の明るさ（1 = 外側と同じ） */
				float innerShade = 0.6f;
				/** 縁の色（rgb）と、どれだけ混ぜるか（a）：冷えているとき */
				float edgeColor[4] = { 0.9f, 0.97f, 1.0f, 0.6f };
				/** 縁の色：溶けている最中（1 より大きいとブルームで少し光る） */
				float meltingEdgeColor[4] = { 1.6f, 1.2f, 0.85f, 0.85f };
				/** 鏡に映った像も描くか（GimmickRenderer の isDrawReflection を毎フレーム写す） */
				bool isDrawReflection = true;
			};


		public:
			/** @brief 初期化（モデルは必要になったときに読む） */
			void Init(const Config& config);
			/** @brief 氷に合わせてモデルを置く（毎フレーム、ステージの更新のあとに呼ぶ） */
			void Update(const stage::Stage& stage);
			/** @brief 描く（シェーダーに氷の残り具合を送ってから、描くモデルを登録する） */
			void Draw(RenderContext& rc);
			/** @brief 設定（ImGui での調整用） */
			inline Config& GetConfig() { return m_config; }
			/** @brief 今フレーム描くモデルの数（本物 + 鏡に映った像） */
			size_t GetDrawCount() const;


		public:
			IceRenderer();
			// Model を持っているのでコピー禁止
			IceRenderer(const IceRenderer&) = delete;
			IceRenderer& operator=(const IceRenderer&) = delete;


		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す値（ice.fx の DirectionLightCb と同じ並び）
			 * @details 前半は model.fx と同じライトの値。その後ろに氷の値を float4 でつなげる。
			 */
			struct Constants
			{
				SceneLightData light;
				float iceRect[4];		// 氷の範囲：xy = 左下, zw = 右上（ワールド）
				float iceGrid[4];		// x = 列の数, y = 行の数, z = マスの幅, w = マスの高さ
				float iceParam[4];		// x = 描かない残り具合（threshold）, y = 縁の幅, z = 内側の明るさ, w = 鏡に映った像なら 1
				float iceEdgeColor[4];	// rgb = 縁の色, a = どれだけ混ぜるか
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");

			/**
			 * @brief モデル1つと、そのモデル専用のシェーダーに渡す値
			 */
			struct IceModel
			{
				Model model;
				Constants constants;
			};

			/**
			 * @brief 氷1つぶんの描画用の入れ物
			 * @details マスの数や種類が変わったら作り直す（構造化バッファの大きさがマスの数で決まるため）。
			 */
			struct IceView
			{
				/** マスの数（横・縦） */
				int columns = 0;
				int rows = 0;
				/** 守る氷か */
				bool isKeep = false;
				/** 今フレーム描くか */
				bool isVisible = false;
				/** 今フレーム「溶けている最中」のモデルを使うか */
				bool isMelting = false;
				/** 今フレーム鏡に映った像も描くか */
				bool isReflected = false;
				/** マスごとの残り具合（CPU 側。Draw で GPU に送る） */
				std::vector<float> field;
				/** マスごとの残り具合（シェーダーの t11） */
				StructuredBuffer fieldBuffer;
				/** モデル [溶けている最中か][鏡に映った像か]（使うときに作る） */
				std::unique_ptr<IceModel> models[2][2];
			};


		private:
			/** @brief index 番目の氷の入れ物を取り出す（マスの数や種類が変わっていたら作り直す） */
			IceView& AcquireView(size_t index, const stage::IceBlock& ice);
			/** @brief モデルを取り出す（なければ作る） */
			IceModel& AcquireModel(IceView& view, bool isMelting, bool isReflected);
			/** @brief シェーダーに渡す値を入れる */
			void SetConstants(Constants& constants, const stage::IceBlock& ice, const IceView& view, bool isMelting, bool isReflected) const;


		private:
			/** 設定 */
			Config m_config;
			/** 氷ごとの入れ物（ステージの氷の順。unique_ptr なので、増えても中身の場所は変わらない。Model はこの場所を覚えている） */
			std::vector<std::unique_ptr<IceView>> m_views;
			/** 今フレーム使っている数 */
			size_t m_viewCount;
			/** 初期化済みか */
			bool m_isInited;
		};
	}
}
