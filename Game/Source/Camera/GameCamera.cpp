/**
 * @file GameCamera.cpp
 * @brief インゲームのカメラ
 */
#include "stdafx.h"
#include "GameCamera.h"

#include <cmath>


namespace
{
	/** 近平面と遠平面の最低限の間隔 */
	constexpr float MIN_CLIP_GAP = 1.0f;
	/** 画面サイズが取れなかったときのアスペクト比 */
	constexpr float DEFAULT_ASPECT = 16.0f / 9.0f;
	/** 全体表示（透視投影）で遠くまで引いたとき、遠平面をカメラの距離の何倍より奥にするか（ステージが切れないように） */
	constexpr float OVERVIEW_FAR_RATE = 2.0f;

	/** value を minValue〜maxValue の間に収める */
	float ClampFloat(const float value, const float minValue, const float maxValue)
	{
		if (value < minValue)
		{
			return minValue;
		}
		if (value > maxValue)
		{
			return maxValue;
		}
		return value;
	}
}


namespace app
{
	namespace camera
	{
		GameCamera::GameCamera()
			: m_camera(nullptr)
			, m_parameter()
			, m_initialParameter()
			, m_currentTarget(0.0f, 0.0f, 0.0f)
			, m_currentDistance(0.0f)
			, m_currentZoom(1.0f)
			, m_stageBounds()
			, m_hasStageBounds(false)
		{}


		void GameCamera::Init(Camera* camera, const CameraParameter& parameter)
		{
			m_camera = camera;
			m_parameter = parameter;
			m_initialParameter = parameter;
			m_currentDistance = ClampFloat(m_parameter.followDistance, limit::MIN_DISTANCE, limit::MAX_DISTANCE);
			m_currentZoom = ClampFloat(m_parameter.zoom, limit::MIN_ZOOM, limit::MAX_ZOOM);
			if (m_camera != nullptr)
			{
				ApplyProjection();
			}
		}


		void GameCamera::Update(const float deltaTime, const Vector3& followPosition)
		{
			// なめらかに追いかける（フレームレートが変わっても同じ速さになる書き方）
			const float rate = 1.0f - std::exp(-m_parameter.smoothing * deltaTime);
			Follow(followPosition, rate);
		}


		void GameCamera::SnapTo(const Vector3& followPosition)
		{
			Follow(followPosition, 1.0f);
		}


		void GameCamera::SetStageBounds(const math::Rect& bounds)
		{
			m_stageBounds = bounds;
			m_hasStageBounds = bounds.Width() > 0.0f && bounds.Height() > 0.0f;
		}


		float GameCamera::GetAspect() const
		{
			const float frameWidth = static_cast<float>(g_graphicsEngine->GetFrameBufferWidth());
			const float frameHeight = static_cast<float>(g_graphicsEngine->GetFrameBufferHeight());
			return frameHeight > 0.0f ? frameWidth / frameHeight : DEFAULT_ASPECT;
		}


		void GameCamera::Follow(const Vector3& followPosition, const float rate)
		{
			// Init の前に呼ばれたら何もしない
			if (m_camera == nullptr)
			{
				return;
			}

			Vector3 desiredTarget;
			float desiredZoom = 1.0f;
			float desiredDistance = 0.0f;
			CalcDesiredView(followPosition, desiredTarget, desiredZoom, desiredDistance);

			m_currentTarget.x += (desiredTarget.x - m_currentTarget.x) * rate;
			m_currentTarget.y += (desiredTarget.y - m_currentTarget.y) * rate;
			m_currentTarget.z += (desiredTarget.z - m_currentTarget.z) * rate;
			m_currentDistance += (desiredDistance - m_currentDistance) * rate;
			// ズームも同じ速さで近づける（全体表示の ON / OFF で、すっと引いたり寄ったりする）
			m_currentZoom += (desiredZoom - m_currentZoom) * rate;

			// ImGui で変えた投影の値や、画面サイズの変化も反映されるよう毎回設定し直す
			ApplyProjection();
			ApplyView();
		}


		void GameCamera::CalcDesiredView(const Vector3& followPosition, Vector3& outTarget, float& outZoom, float& outDistance) const
		{
			// ステージの範囲がないときは、全体表示にしていても、ふだんの見え方にする
			if (!m_parameter.isOverview || !m_hasStageBounds)
			{
				// ふだん：見たい位置（足元）の少し上を、決めたズーム・距離で映す
				outTarget = Vector3(followPosition.x, followPosition.y + m_parameter.targetOffsetY, followPosition.z);
				outZoom = ClampFloat(m_parameter.zoom, limit::MIN_ZOOM, limit::MAX_ZOOM);
				outDistance = ClampFloat(m_parameter.followDistance, limit::MIN_DISTANCE, limit::MAX_DISTANCE);
				return;
			}

			// 全体表示：ステージの範囲 + 余白 が、縦にも横にも収まる広さにする
			const float margin = m_parameter.overviewMargin > 0.0f ? m_parameter.overviewMargin : 0.0f;
			const float needWidth = m_stageBounds.Width() + margin * 2.0f;
			const float needHeight = m_stageBounds.Height() + margin * 2.0f;
			// 横に長いステージは「横の長さ ÷ アスペクト比」の縦の長さが必要になる（大きいほうに合わせる）
			const float aspect = GetAspect();
			const float viewHeight = needHeight > needWidth / aspect ? needHeight : needWidth / aspect;

			// ステージの真ん中を見る（奥行き z だけは渡された値を使う）
			outTarget = Vector3(
				(m_stageBounds.min.x + m_stageBounds.max.x) * 0.5f,
				(m_stageBounds.min.y + m_stageBounds.max.y) * 0.5f,
				followPosition.z);

			// 平行投影：縦に映る長さ = baseViewHeight × ズーム
			const float baseViewHeight = m_parameter.baseViewHeight > 0.0f ? m_parameter.baseViewHeight : CameraParameter().baseViewHeight;
			outZoom = ClampFloat(viewHeight / baseViewHeight, limit::MIN_ZOOM, limit::MAX_OVERVIEW_ZOOM);

			if (m_parameter.isOrtho)
			{
				// 平行投影では距離で見える範囲は変わらないので、ふだんの距離のまま
				outDistance = ClampFloat(m_parameter.followDistance, limit::MIN_DISTANCE, limit::MAX_DISTANCE);
			}
			else
			{
				// 透視投影：縦の画角で viewHeight がちょうど入る距離まで離れる
				const float viewAngle = ClampFloat(m_parameter.viewAngleDeg, limit::MIN_VIEW_ANGLE, limit::MAX_VIEW_ANGLE);
				const float halfTan = std::tan(Math::DegToRad(viewAngle) * 0.5f);
				const float distance = halfTan > 0.0f ? viewHeight * 0.5f / halfTan : limit::MAX_OVERVIEW_DISTANCE;
				outDistance = ClampFloat(distance, limit::MIN_DISTANCE, limit::MAX_OVERVIEW_DISTANCE);
			}
		}


		void GameCamera::ApplyProjection()
		{
			// 近平面と遠平面（遠平面が近平面より手前にならないようにする）
			const float nearClip = m_parameter.nearClip > 0.0f ? m_parameter.nearClip : CameraParameter().nearClip;
			float farClip = m_parameter.farClip > nearClip + MIN_CLIP_GAP ? m_parameter.farClip : nearClip + MIN_CLIP_GAP;
			// 遠くまで引いたときにステージが遠平面で切れないようにする（透視投影の全体表示のとき）
			const float minFarClip = m_currentDistance * OVERVIEW_FAR_RATE;
			if (farClip < minFarClip)
			{
				farClip = minFarClip;
			}
			m_camera->SetNear(nearClip);
			m_camera->SetFar(farClip);

			if (m_parameter.isOrtho)
			{
				// 平行投影：縦に映る範囲を今のズームで決めて、横は画面のアスペクト比に合わせる（横に伸びないように）
				const float viewHeight = GetCurrentViewHeight();

				m_camera->SetUpdateProjMatrixFunc(Camera::enUpdateProjMatrixFunc_Ortho);
				m_camera->SetHeight(viewHeight);
				m_camera->SetWidth(viewHeight * GetAspect());
			}
			else
			{
				// 透視投影：画角で見える範囲が決まる（距離を離すほど引いた視点になる）
				const float viewAngle = ClampFloat(m_parameter.viewAngleDeg, limit::MIN_VIEW_ANGLE, limit::MAX_VIEW_ANGLE);

				m_camera->SetUpdateProjMatrixFunc(Camera::enUpdateProjMatrixFunc_Perspective);
				m_camera->SetViewAngle(Math::DegToRad(viewAngle));
			}
		}


		void GameCamera::ApplyView()
		{
			// 手前(-Z)から、少し上から見る
			m_camera->SetTarget(m_currentTarget);
			m_camera->SetPosition(Vector3(
				m_currentTarget.x,
				m_currentTarget.y + m_parameter.height,
				m_currentTarget.z - m_currentDistance));
		}
	}
}
