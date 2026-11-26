#pragma once

namespace nsK2EngineLow
{
	/**
	 * @brief 画像のまわりに付ける縁（文字を、明るい背景や込み入った背景の上でも読みやすくする）
	 */
	struct SpriteOutline
	{
		/** rgb = 縁の色, a = 縁の濃さ */
		Vector4 color;
		/** 縁の太さ（画面のピクセル。拡大率 1 のとき） */
		float thickness = 0.0f;
	};


	class SpriteRender
	{
	public:

		void Init(const char* ddsFilePath, const float width, const float height);
		/**
		 * @brief 縁を付けて出す画像として読み込む（Assets/shader/spriteOutline.fx）
		 * @details 縁が画像の外にはみ出しても切れないように、板を縁の太さの分だけ広げて描く。
		 *          位置・拡大率・色（SetMulColor）の使い方は Init と同じ。縁も乗算カラーの透明度で一緒に透ける。
		 */
		void InitWithOutline(const char* ddsFilePath, const float width, const float height, const SpriteOutline& outline);

		void SetPosition(const Vector3& pos)
		{
			m_position = pos;
		}

		void SetRotation(const Quaternion& rot)
		{
			m_rotation = rot;
		}

		void SetScale(const Vector3& scale)
		{
			m_scale = scale;
		}

		void SetMulColor(const Vector4& mulColor)
		{
			m_sprite.SetMulColor(mulColor);
		}
		void SetPivot(const Vector2& pivot)
		{
			m_pivot = pivot;
		}

		void Update();

		void Draw(RenderContext& rc);


	private:
		/**
		 * @brief 縁のシェーダーに渡す値（spriteOutline.fx の OutlineCb と同じ並び）
		 */
		struct OutlineValues
		{
			/** rgb = 縁の色, a = 縁の濃さ */
			Vector4 color;
			/** xy = 縁の太さ（画像の幅・高さに対する割合）, zw = 板を広げた倍率（板の大きさ ÷ 画像の大きさ） */
			Vector4 param;
		};

	private:

		Sprite m_sprite;
		/** 縁のシェーダーに渡す値（InitWithOutline のときだけ使う。Sprite がこの場所を覚えるので、コピーしない） */
		OutlineValues m_outlineValues;
		Vector3 m_position = Vector3::Zero;
		Quaternion m_rotation = Quaternion::Identity;
		Vector3 m_scale = Vector3::One;
		Vector2 m_pivot = Sprite::DEFAULT_PIVOT;

	};
}