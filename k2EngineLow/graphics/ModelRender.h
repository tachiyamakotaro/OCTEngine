#pragma once


namespace nsK2EngineLow
{
	class ModelRender
	{
	public:
		void Init(const char* tkmFilePath, const char* fxFilePath, void* expandConstantBuffer, const int expandConstantBufferSize);

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
		Model m_model;

		Vector3 m_position = Vector3::Zero;
		Quaternion m_rotation = Quaternion::Identity;
		Vector3 m_scale = Vector3::One;
	};
}