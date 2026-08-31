#pragma once


namespace nsK2EngineLow
{
	class ModelRender
	{
	public:
		void Init(const char* tkmFilePath, bool shadowCaster = false, bool shadowReceiver = false, void* expandConstantBuffer = nullptr, const int expandConstantBufferSize = 0);

		void SetPosition(const Vector3& pos)
		{
			m_position = pos;
		}

		Vector3 GetPosition() const
		{
			return m_position;
		}

		void SetRotation(const Quaternion& rot)
		{
			m_rotation = rot;
		}

		Quaternion GetRotation() const
		{
			return m_rotation;
		}

		void SetScale(const Vector3& scale)
		{
			m_scale = scale;
		}

		Vector3 GetScale() const
		{
			return m_scale;
		}

		void Update();

		void Draw(RenderContext& rc);


	private:
		Model m_model;

		Vector3 m_position = Vector3::Zero;
		Quaternion m_rotation = Quaternion::Identity;
		Vector3 m_scale = Vector3::One;

		bool m_shadowCaster = false;
		bool m_shadowReceiver = false;
	};
}