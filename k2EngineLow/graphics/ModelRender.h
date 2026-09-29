#pragma once


namespace nsK2EngineLow
{
	class ModelRender
	{
	public:
		void Init(const char* tkmFilePath, AnimationClip* animationClips = nullptr, int numAnimationClips = 0, bool shadowCaster = false, bool shadowReceiver = false, void* expandConstantBuffer = nullptr, const int expandConstantBufferSize = 0, EnModelUpAxis modelUpAxis = enModelUpAxisZ);

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

		void PlayAnimation(int animNo, float interpolateTime = 0.0f)
		{
			m_animation.Play(animNo, interpolateTime);
		}

		void SetAnimationSpeed(float speed)
		{
			m_animationSpeed = speed;
		}

		void Update();

		void Draw(RenderContext& rc);


	private:
		Model m_model;
		Model m_shadowModel;
		AnimationClip* m_animationClip = nullptr;
		Animation m_animation;
		Skeleton m_skeleton;

		Vector3 m_position = Vector3::Zero;
		Quaternion m_rotation = Quaternion::Identity;
		Vector3 m_scale = Vector3::One;

		int m_numAnimationClip = 0;
		float m_animationSpeed = 1.0f;
		bool m_isAnimated = false;
		bool m_shadowCaster = false;
		bool m_shadowReceiver = false;
	};
}