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
			, m_eventShot()
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

			const DesiredView desired = CalcDesiredView(followPosition);

			m_currentTarget.x += (desired.target.x - m_currentTarget.x) * rate;
			m_currentTarget.y += (desired.target.y - m_currentTarget.y) * rate;
			m_currentTarget.z += (desired.target.z - m_currentTarget.z) * rate;
			m_currentDistance += (desired.distance - m_currentDistance) * rate;
			// ズームも同じ速さで近づける（全体表示の ON / OFF・演出で、すっと引いたり寄ったりする）
			m_currentZoom += (desired.zoom - m_currentZoom) * rate;

			// ImGui で変えた投影の値や、画面サイズの変化も反映されるよう毎回設定し直す
			ApplyProjection();
			ApplyView();
		}


		GameCamera::DesiredView GameCamera::CalcDesiredView(const Vector3& followPosition) const
		{
			// ステージの範囲がないときは、全体表示にしていても、ふだんの見え方にする
			if (m_parameter.isOverview && m_hasStageBounds)
			{
				// 全体表示のときは演出で寄らない（動画の撮影用なので、ステージ全体を映したままにする）
				return CalcOverviewView(followPosition);
			}
			const DesiredView followView = CalcFollowView(followPosition);
			if (!m_eventShot.IsActive())
			{
				return followView;
			}
			return BlendEventShot(followView);
		}


		GameCamera::DesiredView GameCamera::CalcFollowView(const Vector3& followPosition) const
		{
			DesiredView view;
			view.target = Vector3(followPosition.x, followPosition.y + m_parameter.targetOffsetY, followPosition.z);
			view.zoom = ClampFloat(m_parameter.zoom, limit::MIN_ZOOM, limit::MAX_ZOOM);
			view.distance = ClampFloat(m_parameter.followDistance, limit::MIN_DISTANCE, limit::MAX_DISTANCE);
			return view;
		}


		GameCamera::DesiredView GameCamera::CalcOverviewView(const Vector3& followPosition) const
		{
			// ステージの範囲 + 余白 が、縦にも横にも収まる広さにする
			const float viewHeight = CalcFitViewHeight(m_stageBounds.Expanded(m_parameter.overviewMargin));

			DesiredView view;
			// ステージの真ん中を見る（奥行き z だけは渡された値を使う）
			const math::Vec2 center = m_stageBounds.Center();
			view.target = Vector3(center.x, center.y, followPosition.z);

			// 平行投影：縦に映る長さ = baseViewHeight × ズーム
			const float baseViewHeight = m_parameter.baseViewHeight > 0.0f ? m_parameter.baseViewHeight : CameraParameter().baseViewHeight;
			view.zoom = ClampFloat(viewHeight / baseViewHeight, limit::MIN_ZOOM, limit::MAX_OVERVIEW_ZOOM);

			if (m_parameter.isOrtho)
			{
				// 平行投影では距離で見える範囲は変わらないので、ふだんの距離のまま
				view.distance = ClampFloat(m_parameter.followDistance, limit::MIN_DISTANCE, limit::MAX_DISTANCE);
			}
			else
			{
				// 透視投影：縦の画角で viewHeight がちょうど入る距離まで離れる
				const float viewAngle = ClampFloat(m_parameter.viewAngleDeg, limit::MIN_VIEW_ANGLE, limit::MAX_VIEW_ANGLE);
				const float halfTan = std::tan(Math::DegToRad(viewAngle) * 0.5f);
				const float distance = halfTan > 0.0f ? viewHeight * 0.5f / halfTan : limit::MAX_OVERVIEW_DISTANCE;
				view.distance = ClampFloat(distance, limit::MIN_DISTANCE, limit::MAX_OVERVIEW_DISTANCE);
			}
			return view;
		}


		GameCamera::DesiredView GameCamera::BlendEventShot(const DesiredView& followView) const
		{
			const float weight = ClampFloat(m_eventShot.weight, 0.0f, 1.0f);
			const math::Rect& area = m_eventShot.area;

			// 演出のズーム：決めた寄り具合まで寄る。ただし範囲が入りきらないときは入るところまで引き、ふだんより引くことはない
			const float baseViewHeight = m_parameter.baseViewHeight > 0.0f ? m_parameter.baseViewHeight : CameraParameter().baseViewHeight;
			const float fitZoom = CalcFitViewHeight(area) / baseViewHeight;
			const float wantZoom = m_eventShot.zoom > fitZoom ? m_eventShot.zoom : fitZoom;
			const float eventZoom = ClampFloat(wantZoom, limit::MIN_ZOOM, followView.zoom);

			// 演出の注視点：範囲の真ん中（奥行き z はふだんのまま）
			// ギミックが遠くて範囲が入りきらないときも、プレイヤー（keepArea）は画面から出さない
			const math::Vec2 center = KeepAreaInView(area.Center(), m_eventShot.keepArea, baseViewHeight * eventZoom);

			DesiredView view;
			view.target = Vector3(
				followView.target.x + (center.x - followView.target.x) * weight,
				followView.target.y + (center.y - followView.target.y) * weight,
				followView.target.z);
			view.zoom = followView.zoom + (eventZoom - followView.zoom) * weight;
			// 透視投影はズームではなく距離で見える範囲が決まるので、ズームの比の分だけ近づける（平行投影では見え方は変わらない）
			const float zoomRate = followView.zoom > 0.0f ? view.zoom / followView.zoom : 1.0f;
			view.distance = ClampFloat(followView.distance * zoomRate, limit::MIN_DISTANCE, limit::MAX_DISTANCE);
			return view;
		}


		float GameCamera::CalcFitViewHeight(const math::Rect& area) const
		{
			// 横に長い範囲は「横の長さ ÷ アスペクト比」の縦の長さが必要になる（大きいほうに合わせる）
			const float aspect = GetAspect();
			const float heightForWidth = area.Width() / aspect;
			return area.Height() > heightForWidth ? area.Height() : heightForWidth;
		}


		math::Vec2 GameCamera::KeepAreaInView(const math::Vec2& center, const math::Rect& keepArea, const float viewHeight) const
		{
			const float halfHeight = viewHeight * 0.5f;
			const float halfWidth = halfHeight * GetAspect();
			const math::Vec2 keepCenter = keepArea.Center();

			// 1つの軸ぶん：真ん中が [keepの右端 - 画面の半分, keepの左端 + 画面の半分] に入るようにする
			// （keepArea のほうが画面より大きいときは、keepArea の真ん中を見る）
			const auto clampAxis = [](const float value, const float keepMin, const float keepMax, const float half, const float keepMid)
				{
					const float low = keepMax - half;
					const float high = keepMin + half;
					if (low > high)
					{
						return keepMid;
					}
					return ClampFloat(value, low, high);
				};
			return math::Vec2(
				clampAxis(center.x, keepArea.min.x, keepArea.max.x, halfWidth, keepCenter.x),
				clampAxis(center.y, keepArea.min.y, keepArea.max.y, halfHeight, keepCenter.y));
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
