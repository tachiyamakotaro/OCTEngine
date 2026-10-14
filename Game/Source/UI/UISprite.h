/**
 * @file UISprite.h
 * @brief メニュー画面で使う画像1枚（SpriteRender と表示する大きさのセット）と、メニュー共通の色
 * @details タイトルメニュー・ポーズ画面・サウンドオプション画面で同じものを使う。
 *          座標は画面の真ん中が原点、上が +Y（画面は 1920x1080）。
 */
#pragma once


namespace app
{
	namespace ui
	{
		/**
		 * @brief メニュー共通の色
		 * @details COLOR_ で始まる名前は Windows のヘッダーのマクロとぶつかることがあるので、名前空間に入れている
		 */
		namespace menu_color
		{
			/** 選んでいる項目（黄色） */
			inline const Vector4 Selected(1.0f, 0.82f, 0.05f, 1.0f);
			/** 選んでいない項目（暗い背景の上で使う明るい灰色） */
			inline const Vector4 Normal(0.75f, 0.75f, 0.78f, 1.0f);
			/** そのままの色 */
			inline const Vector4 White(1.0f, 1.0f, 1.0f, 1.0f);
			/** 音量の目盛りの消えている部分 */
			inline const Vector4 BlockOff(0.25f, 0.25f, 0.28f, 0.9f);
		}


		/**
		 * @brief メニュー用の画像1枚
		 */
		struct UISprite
		{
			SpriteRender render;
			/** 表示する大きさ（拡大率 1 のとき） */
			float width = 0.0f;
			float height = 0.0f;

			/**
			 * @brief 画像を読み込む（表示する高さに合わせて、元の縦横比のまま幅を決める）
			 * @param pixelWidth  画像の元の幅（ピクセル。縦横比を出すためだけに使う）
			 * @param pixelHeight 画像の元の高さ（ピクセル）
			 * @param displayHeight 画面に出す高さ
			 */
			void Init(const char* path, const float pixelWidth, const float pixelHeight, const float displayHeight)
			{
				height = displayHeight;
				width = displayHeight * pixelWidth / pixelHeight;
				render.Init(path, width, height);
			}

			/** 画面いっぱいに出す画像として読み込む */
			void InitFullScreen(const char* path)
			{
				width = static_cast<float>(FRAME_BUFFER_W);
				height = static_cast<float>(FRAME_BUFFER_H);
				render.Init(path, width, height);
			}

			/** 置く（位置・拡大率・色・Z 回転をまとめて設定して Update する） */
			void Place(const float x, const float y, const float scale, const Vector4& color, const float rotationDeg = 0.0f)
			{
				Quaternion rotation;
				rotation.SetRotationDegZ(rotationDeg);
				render.SetPosition(Vector3(x, y, 0.0f));
				render.SetRotation(rotation);
				render.SetScale(Vector3(scale, scale, 1.0f));
				render.SetMulColor(color);
				render.Update();
			}

			/** 描く */
			void Draw(RenderContext& rc)
			{
				render.Draw(rc);
			}
		};
	}
}
