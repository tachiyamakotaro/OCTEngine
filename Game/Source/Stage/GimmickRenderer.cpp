/**
 * @file GimmickRenderer.cpp
 * @brief ステージのギミックをモデルで描く
 */
#include "stdafx.h"
#include "GimmickRenderer.h"
#include "Source/Render/MirrorReflection.h"
#include "Stage.h"
#include <cmath>


namespace app
{
	namespace stage
	{
		ModelRender& GimmickRenderer::ModelPool::Acquire()
		{
			if (usedCount >= models.size())
			{
				auto model = std::make_unique<ModelRender>();
				model->Init(
					path.c_str(), nullptr, 0,
					false, true,
					&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
					upAxis,
					cullMode);
				models.push_back(std::move(model));
			}
			return *models[usedCount++];
		}


		void GimmickRenderer::Init(const Config& config)
		{
			m_config = config;
			for (int i = 0; i < static_cast<int>(ModelKind::Num); ++i)
			{
				m_pools[i] = ModelPool();
				m_pools[i].path = config.modelPaths[i];
				m_pools[i].upAxis = config.upAxis;
				m_pools[i].cullMode = D3D12_CULL_MODE_BACK;

				m_reflectionPools[i] = ModelPool();
				m_reflectionPools[i].path = config.modelPaths[i];
				m_reflectionPools[i].upAxis = config.upAxis;
				m_reflectionPools[i].cullMode = D3D12_CULL_MODE_FRONT;
			}
			m_isInited = true;
		}


		void GimmickRenderer::Update(const Stage& stage)
		{
			if (!m_isInited)
			{
				return;
			}
			for (int i = 0; i < static_cast<int>(ModelKind::Num); ++i)
			{
				m_pools[i].usedCount = 0;
				m_reflectionPools[i].usedCount = 0;
			}

			const BackMirror& backMirror = stage.GetBackMirror();
			m_isMirrorEnabled = backMirror.isEnabled && m_config.isDrawReflection;
			m_mirrorPlaneZ = backMirror.planeZ;

			// 扉：今開いている分だけ下から縮んでいく（上に引っ込むシャッター）
			for (const Door& door : stage.GetGimmicks().View<Door>())
			{
				if (door.IsSolid())
				{
					PutRect(ModelKind::Door, door.GetCurrentRect(), m_config.doorDepth);
				}
			}

			// 受光器：点灯と消灯でモデルを切り替える
			for (const LightReceiver& receiver : stage.GetGimmicks().View<LightReceiver>())
			{
				PutRect(receiver.isLit ? ModelKind::ReceiverOn : ModelKind::ReceiverOff, receiver.rect, m_config.receiverDepth);
			}

			// 光の足場：出ているときだけ
			for (const LightPlatform& platform : stage.GetGimmicks().View<LightPlatform>())
			{
				if (platform.IsSolid())
				{
					PutRect(ModelKind::LightPlatform, platform.rect, m_config.lightPlatformDepth);
				}
			}

			// 光源：光の出る面(+X)を光の向きに回す
			for (const EmitRay& source : stage.GetGimmicks().View<EmitRay>())
			{
				const float size = m_config.lightSourceSize;
				Put(ModelKind::LightSource, source.position, 0.0f, source.angleDeg, Vector3(size, size, m_config.lightSourceDepth));
			}

			// 反射鏡：表(+Y)が鏡の法線を向くように回す
			for (const LightMirror& mirror : stage.GetGimmicks().View<LightMirror>())
			{
				Put(ModelKind::LightMirror, mirror.center, 0.0f, mirror.angleDeg,
					Vector3(mirror.length, m_config.lightMirrorThickness, m_config.lightMirrorDepth));
			}

			// 光は半透明にするため、LightBeamRenderer が描く（ここでは描かない）

			// 床スイッチ：押されると低くつぶれて色が変わる（下端はそのまま）
			for (const FloorSwitch& floorSwitch : stage.GetGimmicks().View<FloorSwitch>())
			{
				math::Rect rect = floorSwitch.rect;
				if (floorSwitch.isPressed)
				{
					rect.max.y = rect.min.y + rect.Height() * m_config.floorSwitchPressedScale;
				}
				PutRect(floorSwitch.isPressed ? ModelKind::FloorSwitchOn : ModelKind::FloorSwitchOff, rect, m_config.floorSwitchDepth);
			}

			// 霧の吹き出し口：天井に並べる
			PutMistVents(stage);

			// 鏡の足場：霧に触れて見えるようになってから（見えないうちは風景を映しているので描かない）
			for (const InvisibleFloor& platform : stage.GetGimmicks().View<InvisibleFloor>())
			{
				if (platform.isRevealed)
				{
					PutRect(ModelKind::MirrorPlatform, platform.rect, m_config.mirrorPlatformDepth);
				}
			}

			// 合図のランプ：信号がONなら点灯モデル
			for (const Indicator& indicator : stage.GetGimmicks().View<Indicator>())
			{
				PutRect(indicator.isOn ? ModelKind::IndicatorOn : ModelKind::IndicatorOff, indicator.rect, m_config.indicatorDepth);
			}

			// ワープの暗がり（信号で止まっているワープは描かない）
			for (const WarpZone& zone : stage.GetGimmicks().View<WarpZone>())
			{
				if (zone.disabledBySignal.empty() || !stage.IsSignalOn(zone.disabledBySignal))
				{
					PutRect(ModelKind::WarpGate, zone.rect, m_config.warpGateDepth);
				}
			}

			// ゴール
			for (const Goal& goal : stage.GetGimmicks().View<Goal>())
			{
				PutRect(ModelKind::Goal, goal.rect, m_config.goalDepth);
			}

			// 暗がりの足場：見えている間だけ描く（消えかけのときは暗い色のモデル）
			for (const HiddenFloor& floor : stage.GetGimmicks().View<HiddenFloor>())
			{
				if (floor.IsVisible())
				{
					PutRect(floor.visibility >= 0.5f ? ModelKind::HiddenFloor : ModelKind::HiddenFloorDim, floor.rect, m_config.hiddenFloorDepth);
				}
			}

			// 奥の鏡の枠
			if (backMirror.isEnabled)
			{
				PutMirrorFrame(stage);
			}
		}


		void GimmickRenderer::Draw(RenderContext& rc)
		{
			for (int i = 0; i < static_cast<int>(ModelKind::Num); ++i)
			{
				for (size_t n = 0; n < m_pools[i].usedCount; ++n)
				{
					m_pools[i].models[n]->Draw(rc);
				}
				for (size_t n = 0; n < m_reflectionPools[i].usedCount; ++n)
				{
					m_reflectionPools[i].models[n]->Draw(rc);
				}
			}
		}


		size_t GimmickRenderer::GetPlacedCount() const
		{
			size_t count = 0;
			for (int i = 0; i < static_cast<int>(ModelKind::Num); ++i)
			{
				count += m_pools[i].usedCount + m_reflectionPools[i].usedCount;
			}
			return count;
		}


		void GimmickRenderer::Put(
			const ModelKind kind, const math::Vec2& center, const float z, const float angleDeg,
			const Vector3& size, const bool isReflectable)
		{
			const float unit = m_config.modelSize;
			const Vector3 position(center.x, center.y, z);
			const Vector3 scale(size.x / unit, size.y / unit, size.z / unit);
			Quaternion rotation;
			rotation.SetRotationDegZ(angleDeg);

			const int index = static_cast<int>(kind);
			ModelRender& model = m_pools[index].Acquire();
			model.SetPosition(position);
			model.SetRotation(rotation);
			model.SetScale(scale);
			model.Update();

			if (isReflectable && m_isMirrorEnabled)
			{
				ModelRender& reflected = m_reflectionPools[index].Acquire();
				reflected.SetPosition(render::ReflectPositionZ(position, m_mirrorPlaneZ));
				reflected.SetRotation(render::ReflectRotationZ(rotation));
				reflected.SetScale(render::ReflectScaleZ(scale));
				reflected.Update();
			}
		}


		void GimmickRenderer::PutRect(const ModelKind kind, const math::Rect& rect, const float depth)
		{
			Put(kind, (rect.min + rect.max) * 0.5f, 0.0f, 0.0f, Vector3(rect.Width(), rect.Height(), depth));
		}


		void GimmickRenderer::PutMirrorFrame(const Stage& stage)
		{
			// 鏡の面の上に4本の棒で枠を作る（鏡の面にあるので、鏡には映さない）
			const math::Rect& frame = stage.GetBackMirror().frame;
			const float t = m_config.mirrorFrameThickness;
			const float z = stage.GetBackMirror().planeZ;
			const float width = frame.Width() + t * 2.0f;
			const float height = frame.Height();
			const float centerX = (frame.min.x + frame.max.x) * 0.5f;
			const float centerY = (frame.min.y + frame.max.y) * 0.5f;

			Put(ModelKind::MirrorFrame, { centerX, frame.max.y + t * 0.5f }, z, 0.0f, Vector3(width, t, t), false);
			Put(ModelKind::MirrorFrame, { centerX, frame.min.y - t * 0.5f }, z, 0.0f, Vector3(width, t, t), false);
			Put(ModelKind::MirrorFrame, { frame.min.x - t * 0.5f, centerY }, z, 0.0f, Vector3(t, height, t), false);
			Put(ModelKind::MirrorFrame, { frame.max.x + t * 0.5f, centerY }, z, 0.0f, Vector3(t, height, t), false);
		}


		void GimmickRenderer::PutMistVents(const Stage& stage)
		{
			const float width = m_config.mistVentWidth;
			const float height = m_config.mistVentHeight;
			for (const MistEmitter& emitter : stage.GetGimmicks().View<MistEmitter>())
			{
				// 天井の面から下にぶら下げる（位置は Stage が天井のあるところだけ選ぶ）
				stage.GetMistVentPositions(emitter, m_ventPositions);
				for (const math::Vec2& position : m_ventPositions)
				{
					Put(ModelKind::MistVent, { position.x, position.y - height * 0.5f }, 0.0f, 0.0f, Vector3(width, height, m_config.mistVentDepth));
				}
			}
		}
	}
}
