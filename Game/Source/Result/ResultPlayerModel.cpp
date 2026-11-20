/**
 * @file ResultPlayerModel.cpp
 * @brief 宝物の間のプレイヤー（見た目だけ）
 */
#include "stdafx.h"
#include "ResultPlayerModel.h"
#include "CelebrationTimeline.h"
#include "Source/Core/Math/MathUtil.h"


namespace
{
	/** モデル（ゲームのプレイヤーと同じ。UE から書き出したので Z が上） */
	const char* const MODEL_PATH = "Assets/modelData/player/player.tkm";

	/** アニメーション（ResultPlayerModel::Animation の並び。ファイルはゲームのプレイヤーと同じ） */
	struct AnimationFile
	{
		const char* path;
		bool isLoop;
	};
	constexpr AnimationFile ANIMATION_FILES[] =
	{
		{ "Assets/animData/player/Player_Idle.tka", true },
		{ "Assets/animData/player/Player_Run.tka",  true },
		{ "Assets/animData/player/Player_Jump.tka", false },
	};
	constexpr int ANIMATION_COUNT = static_cast<int>(sizeof(ANIMATION_FILES) / sizeof(ANIMATION_FILES[0]));

	/** モデルの大きさ（PlayerParameter::modelScale と同じ） */
	constexpr float MODEL_SCALE = 4.75f;
	/** 右を向いているときの角度（CharacterBase::GetModelYawDegree と同じ） */
	constexpr float FACE_RIGHT_DEG = 90.0f;
	/** アニメーションを切り替えるときに混ぜる時間 */
	constexpr float BLEND_TIME = 0.15f;
}


namespace app
{
	namespace result
	{
		void ResultPlayerModel::Init(const TreasureRoomLayout& layout)
		{
			static_assert(ANIMATION_COUNT == static_cast<int>(Animation::Num), "ANIMATION_FILES の数が Animation と合っていません");
			m_layout = layout;

			m_animationClips = std::make_unique<AnimationClip[]>(ANIMATION_COUNT);
			for (int i = 0; i < ANIMATION_COUNT; ++i)
			{
				m_animationClips[i].Load(ANIMATION_FILES[i].path);
				m_animationClips[i].SetLoopFlag(ANIMATION_FILES[i].isLoop);
			}
			// ゲームのプレイヤーと同じく、影を落とし、シーンのライトで照らす
			m_modelRender.Init(MODEL_PATH, m_animationClips.get(), ANIMATION_COUNT, true, false,
				&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
				enModelUpAxisZ);

			m_currentAnimation = Animation::Num;
			m_lastHopIndex = -1;
			Play(Animation::Idle);
			m_isInited = true;
		}


		void ResultPlayerModel::Update(const CelebrationTimeline& timeline)
		{
			if (!m_isInited)
			{
				return;
			}

			// ---- 位置（走ってきて止まり、跳ねる） ----
			const float x = math::Lerp(m_layout.playerStartX, m_layout.playerStopX, timeline.GetRunRate());
			const float hopRate = timeline.GetHopRate();
			// 放物線（0 と 1 で床、真ん中でいちばん高い）
			const float y = m_layout.hopHeight * 4.0f * hopRate * (1.0f - hopRate);

			// ---- アニメーション ----
			const int hopIndex = timeline.GetHopIndex();
			if (hopIndex >= 0)
			{
				// 跳ねるたびに Jump を最初から流す
				if (hopIndex != m_lastHopIndex)
				{
					m_modelRender.PlayAnimation(static_cast<int>(Animation::Jump), BLEND_TIME);
					m_currentAnimation = Animation::Jump;
					m_lastHopIndex = hopIndex;
				}
			}
			else
			{
				Play(timeline.IsRunning() ? Animation::Run : Animation::Idle);
			}

			// ---- 向き（右向きから、喜ぶときにカメラのほうへ少し振り向く） ----
			Quaternion rotation;
			rotation.SetRotationDegY(FACE_RIGHT_DEG + m_layout.celebrateTurnDeg * timeline.GetCelebrateRate());

			m_modelRender.SetPosition(Vector3(x, y, 0.0f));
			m_modelRender.SetRotation(rotation);
			m_modelRender.SetScale(Vector3(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE));
			m_modelRender.Update();
		}


		void ResultPlayerModel::Draw(RenderContext& rc)
		{
			if (!m_isInited)
			{
				return;
			}
			m_modelRender.Draw(rc);
		}


		void ResultPlayerModel::Play(const Animation animation)
		{
			if (animation == m_currentAnimation)
			{
				return;
			}
			m_modelRender.PlayAnimation(static_cast<int>(animation), BLEND_TIME);
			m_currentAnimation = animation;
		}
	}
}
