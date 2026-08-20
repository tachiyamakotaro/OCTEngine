#pragma once

namespace nsK2EngineLow
{
	class SpriteRender
	{
	public:

		void Init(const char* ddsFilePath, const float width, const float height);

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

		void Update();

		void Draw(RenderContext& rc);


	private:

		Sprite m_sprite;
		Vector3 m_position = Vector3::Zero;
		Quaternion m_rotation = Quaternion::Identity;
		Vector3 m_scale = Vector3::One;

	};
}