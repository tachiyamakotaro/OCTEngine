/**
 * @file TitlePlayerModel.cpp
 * @brief タイトル画面のプレイヤー（見た目だけ）
 */
#include "stdafx.h"
#include "TitlePlayerModel.h"
#include "EntranceTimeline.h"
#include "RuinEntrance.h"
#include "Source/Core/Math/MathUtil.h"


namespace
{
	/** モデル（ゲームのプレイヤーと同じ。UE から書き出したので Z が上） */
	const char* const MODEL_PATH = "Assets/modelData/player/player.tkm";

	/** アニメーション（TitlePlayerModel::Animation の並び。ファイルはゲームのプレイヤーと同じ） */
	struct AnimationFile
	{
		const char* path;
		bool isLoop;
	};
	constexpr AnimationFile ANIMATION_FILES[] =
	{
		{ "Assets/animData/player/Player_Idle.tka", true },
		{ "Assets/animData/player/Player_Run.tka",  true },
	};
	constexpr int ANIMATION_COUNT = static_cast<int>(sizeof(ANIMATION_FILES) / sizeof(ANIMATION_FILES[0]));

	/** モデルの大きさ（PlayerParameter::modelScale と同じ） */
	constexpr float MODEL_SCALE = 4.75f;
	/** 右を向いているときの角度（CharacterBase::GetModelYawDegree と同じ） */
	constexpr float FACE_RIGHT_DEG = 90.0f;
	/** アニメーションを切り替えるときに混ぜる時間 */
	constexpr float BLEND_TIME = 0.2f;
}


namespace app
{
	namespace title
	{
		void TitlePlayerModel::Init(const RuinEntranceLayout& layout)
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
			Play(Animation::Idle);
			m_isInsideGate = false;
			m_isInited = true;
		}


		void TitlePlayerModel::Update(const EntranceTimeline& timeline, const RuinEntrance& entrance)
		{
			if (!m_isInited)
			{
				return;
			}

			// ---- 位置（左の外から走ってきて門の少し手前で待つ。遊ぶステージを決めたら門の前まで走り、扉が開いたら門の中へ入る） ----
			const float waitX = math::Lerp(m_layout.playerStartX, m_layout.playerWaitX, timeline.GetRunRate());
			const float x = math::Lerp(waitX, m_layout.playerStopX, timeline.GetApproachRate());
			const float walkInRate = timeline.GetWalkInRate();
			const float z = math::Lerp(m_layout.playerZ, m_layout.playerEnterZ, walkInRate);
			// 門の足元の段を上る（門の前より手前では地面の高さ 0）
			const float y = entrance.GetFloorHeight(z);
			m_isInsideGate = walkInRate >= 1.0f;

			// ---- アニメーション ----
			Play(timeline.IsRunning() ? Animation::Run : Animation::Idle);

			// ---- 向き（右向きから、門の前に着いたら門のほう（奥）へ向き、門の中へ入るときはまっすぐ奥を向く） ----
			const float faceGateDeg = math::Lerp(FACE_RIGHT_DEG, m_layout.faceGateDeg, timeline.GetTurnRate());
			Quaternion rotation;
			rotation.SetRotationDegY(math::Lerp(faceGateDeg, m_layout.enterGateDeg, timeline.GetWalkInTurnRate()));

			m_modelRender.SetPosition(Vector3(x, y, z));
			m_modelRender.SetRotation(rotation);
			m_modelRender.SetScale(Vector3(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE));
			m_modelRender.Update();
		}


		void TitlePlayerModel::Draw(RenderContext& rc)
		{
			// 門の中へ入りきったら、光の板に隠れているので描かない（影も門のまわりに残さない）
			if (!m_isInited || m_isInsideGate)
			{
				return;
			}
			m_modelRender.Draw(rc);
		}


		void TitlePlayerModel::Play(const Animation animation)
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
