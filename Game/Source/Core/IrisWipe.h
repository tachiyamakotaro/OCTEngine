#pragma once


/**
 * @file IrisWipe.h
 * @brief 丸く閉じるフェード（アイリスアウト。マリオのゴールのように、黒い画面の丸い穴が小さくなっていく）
 */
namespace app
{
	namespace core
	{
		/**
		 * @brief 丸く閉じるフェードの見た目
		 */
		struct IrisStyle
		{
			/** 穴の縁のぼかし（ピクセル。0 にはしない） */
			float edgeSoftness = 3.0f;
			/** 穴の中心を、追いかける位置からどれだけ上にずらすか（ワールドの長さ） */
			float focusOffsetY = 0.0f;
		};


		/**
		 * @brief 丸く閉じるフェード
		 * @details ・画面いっぱいの黒い画像（Assets/sprite/fade.dds。SceneManager のフェードと同じ画像）を手前に置き、
		 *            シェーダー（Assets/shader/irisWipe.fx）で丸い穴をあける
		 *          ・閉じ具合（0 = 開いている 〜 1 = 真っ暗）と、穴の中心にするワールドの位置を毎フレーム渡す
		 *          ・閉じ具合が 0 の間は描かない
		 *          ・真っ暗になったあとは SceneManager のフェード（同じ画像）に引き継ぐので、つなぎ目は見えない
		 */
		class IrisWipe
		{
		public:
			IrisWipe() = default;
			~IrisWipe() = default;
			// Sprite が定数バッファの場所を覚えているのでコピー禁止
			IrisWipe(const IrisWipe&) = delete;
			IrisWipe& operator=(const IrisWipe&) = delete;

			/**
			 * @brief 初期化（画像とシェーダーを読み込む）
			 * @param style 見た目（ResetStyle で戻る値にもなる）
			 */
			void Init(const IrisStyle& style = IrisStyle());
			/**
			 * @brief 穴の大きさと位置を決める（毎フレーム。カメラを動かしたあとに呼ぶ）
			 * @param closeRate 閉じ具合（0 = 開いている 〜 1 = 真っ暗）
			 * @param focus 穴の中心にするワールドの位置（プレイヤーの体の真ん中など）
			 */
			void Update(float closeRate, const Vector3& focus);
			/** 描く（2D。ほかの画像より手前に出したいので、シーンの Render の最後に呼ぶ） */
			void Draw(RenderContext& rc);

			// ---- 調整値 ----
			const IrisStyle& GetStyle() const { return m_style; }
			IrisStyle& GetStyleForEdit() { return m_style; }
			/** 見た目を Init で渡された値に戻す */
			void ResetStyle() { m_style = m_initialStyle; }

		private:
			/**
			 * @brief シェーダーに渡す値（irisWipe.fx の IrisCb と同じ並び）
			 */
			struct IrisValues
			{
				/** xy = 穴の中心（ピクセル。画面の左上が 0）, z = 穴の半径（ピクセル）, w = 縁のぼかし（ピクセル） */
				Vector4 circle;
				/** xy = 画面の大きさ（ピクセル） */
				Vector4 screen;
			};
			static_assert(sizeof(IrisValues) == 16 * 2, "IrisValues は float4 が2つ（irisWipe.fx と同じ並び）");

		private:
			/** ワールドの位置を、画面のピクセルの位置にする（画面の左上が 0、下が +Y） */
			static Vector2 WorldToPixel(const Vector3& world);
			/** 穴の中心から一番遠い画面の角までの長さ（穴が開ききったときの半径） */
			static float CalcOpenRadius(const Vector2& center);

		private:
			/** 画面いっぱいの黒い画像 */
			Sprite m_sprite;
			/** シェーダーに渡す値 */
			IrisValues m_values;
			/** 見た目 */
			IrisStyle m_style;
			/** Init で渡された見た目 */
			IrisStyle m_initialStyle;
			/** 閉じ具合 */
			float m_closeRate = 0.0f;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
