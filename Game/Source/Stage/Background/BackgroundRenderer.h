#pragma once
#include "Source/Core/Math/Rect.h"
#include "Source/Stage/Background/BackgroundLayout.h"
#include "Source/Stage/Background/BackgroundModelPool.h"
#include "Source/Stage/Background/BackgroundParameter.h"
#include <type_traits>
#include <vector>


/**
 * @file BackgroundRenderer.h
 * @brief ステージの奥の背景（遺跡の回廊・いちばん奥の壁・柱の松明）と、ステージの外の黒い塗りつぶしを描く
 */
namespace app
{
	namespace stage
	{
		class Stage;


		/**
		 * @brief ステージの奥の背景を描く
		 * @details ・奥から順に「ステージの外の色の板 → いちばん奥の壁 → 奥の回廊 → 手前の回廊」を置く
		 *            回廊は柱とアーチの区画（Assets/modelData/background/bg_bay.tkm など）を、ステージの範囲に合わせて並べる（BackgroundLayout）
		 *          ・カメラは平行投影なので、奥に置いただけでは遠くに見えない。そこで
		 *              奥の層ほど「小さく・霧で暗く・カメラについてくる（ゆっくり動く）」ようにして、遠くにあるように見せる
		 *          ・ステージの範囲（全部の地形ブロックを囲む矩形）の外は、シェーダーで背景を消して、外の色（黒）の板を見せる
		 *          ・手前の回廊の柱には松明を付ける。炎はゴールの門と同じモデルとシェーダー（goal_flame.tkm / goalGlow.fx）
		 *            暗がり（DarkZone）の中に見える松明は置かない（光も出さない）
		 *            松明の光は、エンジンのポイントライト（4つまで・全部のモデルが使う）は使わず、
		 *            背景のシェーダー（Assets/shader/background.fx）だけで計算する（画面の近くの MAX_TORCH_LIGHT 個まで）
		 *          ・見えている所の区画だけ置く（Model は BackgroundModelPool で使い回す）
		 *          ・調整用の ImGui は BackgroundEditor にある（このクラスは ImGui を使わない）
		 */
		class BackgroundRenderer
		{
		public:
			/** シェーダーに渡す松明の光の数の上限（background.fx の BG_MAX_TORCH_LIGHT と同じにする） */
			static constexpr int MAX_TORCH_LIGHT = 16;

		public:
			BackgroundRenderer() = default;
			~BackgroundRenderer() = default;
			// Model を持っている（Model は定数バッファの場所を覚えている）のでコピー禁止
			BackgroundRenderer(const BackgroundRenderer&) = delete;
			BackgroundRenderer& operator=(const BackgroundRenderer&) = delete;

			/**
			 * @brief 初期化（モデルは必要になったときに読み込む）
			 * @param style 見た目（ResetStyle で戻る値にもなる）
			 */
			void Init(const BackgroundStyle& style = BackgroundStyle());
			/**
			 * @brief 見えている所に背景を置く（毎フレーム。カメラを動かしたあとに呼ぶ）
			 * @param camera 今のカメラ（位置・向き・映す範囲）
			 * @param isOrtho 平行投影か（平行投影のときだけ、奥の層をカメラについてこさせる）
			 * @param deltaTime 炎のゆらめきの時間を進める
			 */
			void Update(const Stage& stage, const Camera& camera, bool isOrtho, float deltaTime);
			/** 描く（不透明。ほかのモデルより先に呼ぶ） */
			void Draw(RenderContext& rc);

			// ---- 調整値 ----
			const BackgroundStyle& GetStyle() const { return m_style; }
			BackgroundStyle& GetStyleForEdit() { return m_style; }
			/** 見た目を Init で渡された値に戻す */
			void ResetStyle() { m_style = m_initialStyle; }

			// ---- 確認用 ----
			/** 今フレーム置いたモデルの数 */
			size_t GetModelCount() const;
			/** 今フレームシェーダーに渡した松明の光の数 */
			int GetTorchLightCount() const { return m_torchLightCount; }

		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief 背景のシェーダーに渡す値（background.fx の MODEL_CB_EXTRA と同じ並び）
			 */
			struct ShaderValues
			{
				/** x = 環境光の明るさ, y = 平行光の明るさ, z = 霧の濃さ, w = 石のテクスチャ1枚ぶんの長さ */
				Vector4 layer;
				/** xy = カメラについてきた分（石の模様をモデルに貼りつけたままにするために引く） */
				Vector4 offset;
				/** rgb = 石の色にかける色（ステージの外の板では、外の色） */
				Vector4 tint;
				/** rgb = 霧の色 */
				Vector4 fogColor;
				/** ステージの範囲（min.x, min.y, max.x, max.y）。この外は描かない */
				Vector4 stageBounds;
				/** xyz = カメラの向き, w = 平行投影なら 1 */
				Vector4 view;
				/** x = 松明の光の数 */
				Vector4 torchCount;
				/** 松明の光（xyz = 位置, w = 届く距離） */
				Vector4 torchPositionRange[MAX_TORCH_LIGHT];
				/** 松明の光（rgb = 色 × 強さ） */
				Vector4 torchColor[MAX_TORCH_LIGHT];
			};
			/** シェーダーに渡す値（前半は model.fx と同じライトの値、後ろに背景の値をつなげる） */
			struct LayerConstants
			{
				SceneLightData light;
				ShaderValues values;
			};
			/** 炎のシェーダーに渡す値（goalGlow.fx と同じ並び。前半は model.fx と同じライトの値） */
			struct FlameConstants
			{
				SceneLightData light;
				/** rgb = まわりの色, a = 光る強さ */
				Vector4 color;
				/** rgb = 真ん中の色, a = 光る強さ */
				Vector4 coreColor;
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");

			/**
			 * @brief 回廊の層1つぶん（並び・モデル・シェーダーに渡す値）
			 * @details モデルは constants の場所を覚えているので、層はメンバーとして持ち、動かさない
			 */
			struct Layer
			{
				/** 柱・アーチ・松明の金具のシェーダーに渡す値 */
				LayerConstants constants;
				/** 炎のシェーダーに渡す値 */
				FlameConstants flameConstants;
				/** 区画 */
				BackgroundModelPool bays;
				/** アーチが崩れた区画 */
				BackgroundModelPool brokenBays;
				/** 松明の金具 */
				BackgroundModelPool torches;
				/** 松明の炎 */
				BackgroundModelPool flames;
				/** 並び */
				BackgroundLayerLayout layout;
				/** 並べたときの値（変わったら並べ直す） */
				BackgroundLayoutSetting layoutSetting;
			};

			/** 松明の光1つ（シェーダーに渡す前の候補） */
			struct TorchLight
			{
				Vector3 position;
				float range = 0.0f;
				Vector3 color;
				/** カメラの注視点からの距離の2乗（近い順に選ぶ） */
				float distanceSq = 0.0f;
			};

			/** カメラから見た範囲（BackgroundRenderer.cpp で定義） */
			struct View;

		private:
			/** 層のモデルの読み込みの設定をする */
			void InitLayer(Layer& layer);
			/** ステージの範囲か並べ方の値が変わったら、並べ直す */
			void RebuildIfChanged(const math::Rect& bounds);
			/** 層の並べ方の値 */
			BackgroundLayoutSetting MakeLayoutSetting(const BackgroundLayerStyle& style, const BackgroundTorchStyle& torch, std::uint32_t seedOffset) const;
			/** ステージの外の色の板を置く（カメラの見ている所いっぱい） */
			void PlaceBackdrop(const View& view);
			/** いちばん奥の壁を置く */
			void PlaceWall(const View& view);
			/**
			 * @brief 回廊の層を置く（区画・松明・炎。松明の光の候補も集める）
			 * @param stage 暗がり（DarkZone）の中の松明は置かない
			 */
			void PlaceLayer(Layer& layer, const BackgroundLayerStyle& style, const BackgroundTorchStyle& torch, const Stage& stage, const View& view);
			/** 近い松明の光を選んで、全部の層のシェーダーに渡す値に入れる */
			void ApplyTorchLights();
			/**
			 * @brief 層のシェーダーに渡す値を入れる（松明の光は ApplyTorchLights で入れる）
			 * @param offset カメラについてきた分
			 * @param textureScale 石の模様の大きさにかける値（小さく置いた層は模様も小さくする）
			 */
			void WriteLayerValues(ShaderValues& values, const BackgroundLayerStyle& style, const math::Vec2& offset, float textureScale, const View& view) const;
			/** 層がカメラについてきた分（平行投影のときだけ） */
			math::Vec2 CalcParallaxOffset(float parallax, const View& view) const;
			/**
			 * @brief 炎のゆらめき（-1〜1）
			 * @param phase 松明ごとにずらす角度
			 */
			float CalcFlicker(float phase, const BackgroundTorchStyle& torch) const;

		private:
			/** 見た目 */
			BackgroundStyle m_style;
			/** Init で渡された見た目（ResetStyle で戻す） */
			BackgroundStyle m_initialStyle;
			/** 手前の回廊 */
			Layer m_near;
			/** 奥の回廊 */
			Layer m_far;
			/** いちばん奥の壁のシェーダーに渡す値 */
			LayerConstants m_wallConstants;
			/** いちばん奥の壁 */
			BackgroundModelPool m_wall;
			/** ステージの外の色の板のシェーダーに渡す値 */
			LayerConstants m_backdropConstants;
			/** ステージの外の色の板 */
			BackgroundModelPool m_backdrop;
			/** 松明の光の候補（毎フレーム使い回す入れ物） */
			std::vector<TorchLight> m_torchLights;
			/** ステージの範囲（並べたときの値） */
			math::Rect m_bounds;
			/** 並べたことがあるか */
			bool m_hasLayout = false;
			/** 今フレーム置いたか（地形が1つもないときは置かない） */
			bool m_isPlaced = false;
			/** 今フレームシェーダーに渡した松明の光の数 */
			int m_torchLightCount = 0;
			/** ゆらめきの時間 */
			float m_time = 0.0f;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
