/**
 * @file StageRenderer.cpp
 * @brief ステージの地形をモデルで描く
 */
#include "stdafx.h"
#include "StageRenderer.h"
#include "Stage.h"
#include "Source/Render/MirrorReflection.h"


namespace app
{
	namespace stage
	{
		void StageRenderer::Init(const Config& config)
		{
			m_config = config;
			m_models.clear();
			m_reflectionModels.clear();
			m_activeCount = 0;
			m_activeReflectionCount = 0;
			m_isInited = true;
		}


		void StageRenderer::Update(const Stage& stage)
		{
			if (!m_isInited)
			{
				return;
			}

			// 描く範囲を集める（地形ブロック + 動く床・動く壁の今の位置）
			m_blockRects.clear();
			for (const StageBlock& block : stage.GetGimmicks().View<StageBlock>())
			{
				m_blockRects.push_back(block.rect);
			}
			for (const MovingBlock& block : stage.GetGimmicks().View<MovingBlock>())
			{
				m_blockRects.push_back(block.GetCurrentRect());
			}
			const size_t blockCount = m_blockRects.size();
			const BackMirror& mirror = stage.GetBackMirror();

			// 本物
			Reserve(m_models, blockCount, D3D12_CULL_MODE_BACK);
			for (size_t i = 0; i < blockCount; ++i)
			{
				Place(*m_models[i], m_blockRects[i], false, 0.0f);
			}
			m_activeCount = blockCount;

			// 鏡に映った像
			m_activeReflectionCount = 0;
			if (mirror.isEnabled)
			{
				Reserve(m_reflectionModels, blockCount, D3D12_CULL_MODE_FRONT);
				for (size_t i = 0; i < blockCount; ++i)
				{
					Place(*m_reflectionModels[i], m_blockRects[i], true, mirror.planeZ);
				}
				m_activeReflectionCount = blockCount;
			}
		}


		void StageRenderer::Draw(RenderContext& rc)
		{
			for (size_t i = 0; i < m_activeCount; ++i)
			{
				m_models[i]->Draw(rc);
			}
			for (size_t i = 0; i < m_activeReflectionCount; ++i)
			{
				m_reflectionModels[i]->Draw(rc);
			}
		}


		void StageRenderer::Place(ModelRender& model, const math::Rect& rect, const bool isReflected, const float planeZ)
		{
			const Vector3& size = m_config.modelSize;
			const float centerX = (rect.min.x + rect.max.x) * 0.5f;

			Vector3 position;
			Vector3 scale;
			if (m_config.isPlane)
			{
				// 板はブロックの上面に置く（モデルの原点は板の中心）
				position = Vector3(centerX, rect.max.y, 0.0f);
				scale = Vector3(rect.Width() / size.x, 1.0f, m_config.depth / size.z);
			}
			else
			{
				// 箱は幅・高さ・奥行きに拡大する
				// 原点が底面なら下端に、箱の中心なら矩形の中心に置く
				const float y = m_config.isPivotAtBottom ? rect.min.y : (rect.min.y + rect.max.y) * 0.5f;
				position = Vector3(centerX, y, 0.0f);
				scale = Vector3(rect.Width() / size.x, rect.Height() / size.y, m_config.depth / size.z);
			}

			// 地形は回転しないので、鏡に映すのは位置と拡大率だけでよい
			if (isReflected)
			{
				position = render::ReflectPositionZ(position, planeZ);
				scale = render::ReflectScaleZ(scale);
			}
			model.SetPosition(position);
			model.SetScale(scale);
			model.Update();
		}


		void StageRenderer::Reserve(std::vector<std::unique_ptr<ModelRender>>& models, const size_t count, const D3D12_CULL_MODE cullMode)
		{
			// ModelRender の初期化は重いので、足りない分だけ作り、余った分は描かないだけにする
			while (models.size() < count)
			{
				auto model = std::make_unique<ModelRender>();
				model->Init(
					m_config.modelPath.c_str(), nullptr, 0,
					false, m_config.isShadowReceiver,
					&SceneLight::GetInstance()->GetSceneLight(), sizeof(SceneLight::GetInstance()->GetSceneLight()),
					m_config.upAxis,
					cullMode);
				models.push_back(std::move(model));
			}
		}
	}
}
