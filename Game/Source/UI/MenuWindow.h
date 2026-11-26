/**
 * @file MenuWindow.h
 * @brief メニューのウィンドウ（背景の上に出すメニューの後ろに置く、角の丸い板）
 * @details 画像を使わず、シェーダー（Assets/shader/menuWindow.fx）で描く。大きさを変えても角や枠の太さは崩れない。
 *          見た目（下から）：影 → 暗い石の色の地 → 見出しの帯（上の端。下に金の線）→ 内側の細い金の線 → 金の枠
 *          見出しの帯の高さを 0 にすると、帯なしのウィンドウ（操作の案内など）になる。
 *          座標は画面の真ん中が原点、上が +Y（画面は 1920x1080）。
 */
#pragma once


namespace app
{
	namespace ui
	{
		/**
		 * @brief ウィンドウの形（拡大率 1 のとき）
		 */
		struct MenuWindowShape
		{
			/** ウィンドウの真ん中 */
			Vector2 center;
			/** ウィンドウの幅と高さ（枠を含む） */
			Vector2 size;
			/** 上の端の見出しの帯の高さ（0 なら帯なし） */
			float headerHeight = 0.0f;

			/** 見出しの帯の真ん中の高さ（見出しの文字を置く Y） */
			float GetHeaderCenterY() const { return center.y + size.y * 0.5f - headerHeight * 0.5f; }
			/** 見出しの帯の下から、ウィンドウの下の端までの真ん中の高さ（中身を置く所の真ん中） */
			float GetBodyCenterY() const { return center.y - headerHeight * 0.5f; }
		};


		/**
		 * @brief メニューのウィンドウ
		 */
		class MenuWindow
		{
		public:
			MenuWindow();
			~MenuWindow();
			// Sprite が定数バッファの場所を覚えているのでコピー禁止
			MenuWindow(const MenuWindow&) = delete;
			MenuWindow& operator=(const MenuWindow&) = delete;

			/**
			 * @brief 形を決めて、シェーダーを読み込む（1回だけ）
			 * @param shape ウィンドウの位置・大きさ・見出しの帯の高さ
			 */
			void Init(const MenuWindowShape& shape);
			/** 描く（メニューの中身より先に呼ぶ） */
			void Draw(RenderContext& rc);


		private:
			/**
			 * @brief シェーダーに渡す値（menuWindow.fx の WindowCb と同じ並び）
			 */
			struct WindowValues
			{
				/** xy = ウィンドウの大きさ, zw = 板の大きさ（影の分だけ大きい） */
				Vector4 size;
				/** x = 角の丸み, y = 枠の太さ, z = 見出しの帯の高さ, w = 影の広がり */
				Vector4 shape;
				/** 地の色（上の端・下の端） */
				Vector4 fillTop;
				Vector4 fillBottom;
				/** 枠と線の色 */
				Vector4 frameColor;
				/** 見出しの帯の色 */
				Vector4 headerColor;
			};
			static_assert(sizeof(WindowValues) == 16 * 6, "WindowValues は float4 が6つ（menuWindow.fx と同じ並び）");


		private:
			/** 板（画面に出す四角） */
			Sprite m_sprite;
			/** シェーダーに渡す値 */
			WindowValues m_values;
			/** 初期化済みか */
			bool m_isInited;
		};
	}
}
