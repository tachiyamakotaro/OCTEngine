/**
 * @file CharacterBase.cpp
 * @brief キャラクターの基底クラス
 */
#include "stdafx.h"
#include "CharacterBase.h"
#include "CharacterParameter.h"
#include "CharacterStateMachine.h"
#include "CharacterStatus.h"
#include "Source/Render/MirrorReflection.h"


namespace app
{
	namespace actor
	{
		void CharacterBase::SetCollisionWorld(const physics::ICollisionWorld* world)
		{
			m_collisionWorld = world;
			m_controller.SetCollisionWorld(world);
		}


		void CharacterBase::PlayAnimation(const int clipNo, const float interpolateTime)
		{
			if (clipNo == m_currentAnimation)
			{
				return;
			}
			m_currentAnimation = clipNo;

			if (m_isModelInited && clipNo >= 0 && clipNo < m_clipNum)
			{
				m_modelRender.PlayAnimation(clipNo, interpolateTime);
				// 鏡に映る像も同じアニメーションを同じタイミングで再生する
				if (m_isReflectionEnabled)
				{
					m_reflectionRender.PlayAnimation(clipNo, interpolateTime);
				}
			}
		}


		void CharacterBase::SetupCharacter()
		{
			const CharacterParameter& param = GetParameter();

			// キャラクターコントローラー
			m_controller.Init(m_position, param.bodyWidth, param.bodyHeight, m_collisionWorld);
			m_controller.SetGravity(param.gravity);
			m_controller.SetMaxFallSpeed(param.maxFallSpeed);

			// ステータス
			m_status = CreateStatus();
			m_status->Setup(param);

			// ステートマシン（Setup で最初のステートの Enter まで呼ばれる）
			m_stateMachine = CreateStateMachine();
			m_stateMachine->Setup();
		}


		void CharacterBase::UpdateLogic(const float deltaTime)
		{
			if (!m_stateMachine)
			{
				return;
			}

			// 1. ステータス（無敵時間などのタイマー）
			m_status->Update(deltaTime);
			// 2. コマンド（入力 or AI）
			UpdateCommand(deltaTime);
			// 3. ステート：速度を決める
			m_stateMachine->UpdateState(deltaTime);
			// 4. 移動と当たり判定
			m_controller.Move(deltaTime);
			// 5. 動いた結果を見てステートを切り替える
			m_stateMachine->UpdateTransition();

			m_position = m_controller.GetPosition();
		}


		bool CharacterBase::Start()
		{
			SetupCharacter();
			UpdateModel();
			return true;
		}


		void CharacterBase::Update()
		{
			UpdateLogic(g_gameTime->GetFrameDeltaTime());
			UpdateModel();
		}


		void CharacterBase::Render(RenderContext& rc)
		{
			if (m_isModelInited)
			{
				m_modelRender.Draw(rc);
				if (m_isReflectionEnabled && m_isReflectionVisible)
				{
					m_reflectionRender.Draw(rc);
				}
			}
		}


		void CharacterBase::InitModel(const ModelData& data)
		{
			m_clipNum = data.clipNum;
			m_animationClips = std::make_unique<AnimationClip[]>(data.clipNum);
			for (int i = 0; i < data.clipNum; ++i)
			{
				m_animationClips[i].Load(data.animationData[i].fileName);
				m_animationClips[i].SetLoopFlag(data.animationData[i].isLoop);
			}

			m_modelRender.Init(
				data.fileName,
				m_animationClips.get(), data.clipNum,
				true, false,
				&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
				data.upAxis);

			if (m_isReflectionEnabled)
			{
				// 鏡に映る像：アニメーションクリップは共有する（キーフレームは読むだけなので共有してよい）
				// 拡大率を負にして描くので表裏が逆になる。表側を消す（FRONT）設定にする。影は落とさない
				m_reflectionRender.Init(
					data.fileName,
					m_animationClips.get(), data.clipNum,
					false, false,
					&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
					data.upAxis,
					D3D12_CULL_MODE_FRONT);
			}
			m_isModelInited = true;

			// 読み込み前に指定されていたアニメーションを反映
			if (m_currentAnimation >= 0 && m_currentAnimation < m_clipNum)
			{
				m_modelRender.PlayAnimation(m_currentAnimation, 0.0f);
				if (m_isReflectionEnabled)
				{
					m_reflectionRender.PlayAnimation(m_currentAnimation, 0.0f);
				}
			}
		}


		void CharacterBase::UpdateModel()
		{
			if (!m_isModelInited)
			{
				return;
			}

			Quaternion rotation;
			rotation.SetRotationDegY(GetModelYawDegree());

			m_modelRender.SetPosition(GetWorldPosition());
			m_modelRender.SetRotation(rotation);
			m_modelRender.Update();

			// 鏡に映る像（見えていないときもアニメーションを進めて、本物とずれないようにする）
			if (m_isReflectionEnabled)
			{
				m_reflectionRender.SetPosition(render::ReflectPositionZ(GetWorldPosition(), m_reflectionPlaneZ));
				m_reflectionRender.SetRotation(render::ReflectRotationZ(rotation));
				m_reflectionRender.SetScale(render::ReflectScaleZ(Vector3(1.0f, 1.0f, 1.0f)));
				m_reflectionRender.Update();
			}
		}


		float CharacterBase::GetModelYawDegree() const
		{
			return (m_facing > 0 ? 90.0f : -90.0f) + GetParameter().modelYawOffsetDeg;
		}


		std::unique_ptr<CharacterStatus> CharacterBase::CreateStatus()
		{
			return std::make_unique<CharacterStatus>();
		}


		CharacterBase::CharacterBase()
			: m_collisionWorld(nullptr)
			, m_facing(1)
			, m_clipNum(0)
			, m_currentAnimation(-1)
			, m_isModelInited(false)
			, m_isReflectionEnabled(false)
			, m_isReflectionVisible(false)
			, m_reflectionPlaneZ(0.0f)
		{}


		// unique_ptr<CharacterStateMachine> 等を破棄するため、完全な型が見えるここで定義する
		CharacterBase::~CharacterBase() = default;
	}
}
