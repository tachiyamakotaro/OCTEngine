/**
 * @file BackgroundRenderer.cpp
 * @brief ステージの奥の背景を描く
 */
#include "stdafx.h"
#include "BackgroundRenderer.h"
#include "Source/Camera/CameraStageBounds.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Stage/Stage.h"
#include <algorithm>
#include <cmath>


namespace
{
	// ---- モデル（Assets/modelData/background/。Tools/gen_background_models.py で作った） ----
	const char* const BAY_MODEL_PATH = "Assets/modelData/background/bg_bay.tkm";
	const char* const BROKEN_BAY_MODEL_PATH = "Assets/modelData/background/bg_bay_broken.tkm";
	const char* const TORCH_MODEL_PATH = "Assets/modelData/background/bg_torch.tkm";
	/** 炎はゴールの門と同じ物 */
	const char* const FLAME_MODEL_PATH = "Assets/modelData/object/goal_flame.tkm";
	/** いちばん奥の壁とステージの外の板（1辺100の箱。地形と同じ） */
	const char* const BOX_MODEL_PATH = "Assets/modelData/Box.tkm";
	constexpr float BOX_MODEL_SIZE = 100.0f;

	// ---- シェーダー ----
	const char* const BACKGROUND_SHADER_PATH = "Assets/shader/background.fx";
	const char* const STONE_PS_ENTRY = "PSMainBackground";
	const char* const PROP_PS_ENTRY = "PSMainBackgroundProp";
	const char* const BACKDROP_PS_ENTRY = "PSMainBackdrop";
	const char* const FLAME_SHADER_PATH = "Assets/shader/goalGlow.fx";
	const char* const FLAME_PS_ENTRY = "PSMainGoalFlame";

	// ---- 置き方 ----
	/** いちばん奥の壁の厚さ */
	constexpr float WALL_DEPTH = 100.0f;
	/** いちばん奥の壁を、ステージの範囲より広げる余白 */
	constexpr float WALL_MARGIN = 400.0f;
	/** ステージの外の色の板の Z（いちばん奥の壁よりさらに奥） */
	constexpr float BACKDROP_Z = 4000.0f;
	/** ステージの外の色の板を、見えている所より広げる倍率 */
	constexpr float BACKDROP_EXPAND = 2.0f;
	/** 見えている所の判定に足す余白（区画の端が画面の端で切れないように） */
	constexpr float VIEW_MARGIN = 200.0f;
	/** 炎の光の位置（炎の根元から、炎の高さのこの割合だけ上・手前） */
	constexpr float LIGHT_HEIGHT_RATE = 0.4f;
	constexpr float LIGHT_FRONT = 10.0f;
	/** 奥の回廊の乱数の種を、手前の回廊からずらす数（同じ場所が崩れないように） */
	constexpr std::uint32_t FAR_SEED_OFFSET = 101u;
	/** 0 で割らないための下限 */
	constexpr float MIN_FORWARD_Z = 0.05f;
	constexpr float MIN_SCALE = 0.1f;

	/** 点が矩形の中にあるか */
	bool Contains(const app::math::Rect& rect, const app::math::Vec2& point)
	{
		return point.x >= rect.min.x && point.x <= rect.max.x && point.y >= rect.min.y && point.y <= rect.max.y;
	}

	/** 2つの矩形が同じか */
	bool IsSameRect(const app::math::Rect& a, const app::math::Rect& b)
	{
		return a.min.x == b.min.x && a.min.y == b.min.y && a.max.x == b.max.x && a.max.y == b.max.y;
	}

	/** 矩形を広げる */
	app::math::Rect Expand(const app::math::Rect& rect, const app::math::Vec2& amount)
	{
		return { rect.min - amount, rect.max + amount };
	}

	/** 矩形の真ん中 */
	app::math::Vec2 CenterOf(const app::math::Rect& rect)
	{
		return (rect.min + rect.max) * 0.5f;
	}

	/**
	 * @brief 暗がり（DarkZone）の中か（暗さが 0 の暗がりは数えない）
	 * @param pointOnStage 地形の面（Z = 0）での位置
	 */
	bool IsInDarkZone(const app::stage::Stage& stage, const app::math::Vec2& pointOnStage)
	{
		for (const app::stage::DarkZone& zone : stage.GetGimmicks().View<app::stage::DarkZone>())
		{
			if (zone.darkness > 0.0f && zone.Contains(pointOnStage))
			{
				return true;
			}
		}
		return false;
	}

	/** float の配列（rgb）→ Vector4 */
	Vector4 ToColor(const float rgb[3], const float a = 1.0f)
	{
		return Vector4(rgb[0], rgb[1], rgb[2], a);
	}
}


namespace app
{
	namespace stage
	{
		/**
		 * @brief カメラから見た範囲（毎フレーム、カメラから作る）
		 */
		struct BackgroundRenderer::View
		{
			/** カメラの位置 */
			Vector3 eye;
			/** カメラの向き */
			Vector3 forward;
			/** 注視点（XY） */
			math::Vec2 target;
			/** 平行投影のとき、映す範囲の半分 */
			math::Vec2 halfSize;
			/** 透視投影のとき、縦の画角の半分の tan */
			float tanHalfAngle = 0.0f;
			/** 透視投影のとき、横 / 縦 */
			float aspect = 1.0f;
			/** 平行投影か */
			bool isOrtho = true;

			/** カメラから作る */
			static View FromCamera(const Camera& camera, const bool isOrtho)
			{
				View view;
				view.eye = camera.GetPosition();
				view.forward = camera.GetForward();
				view.target = math::Vec2(camera.GetTarget().x, camera.GetTarget().y);
				view.halfSize = math::Vec2(camera.GetWidth() * 0.5f, camera.GetHeight() * 0.5f);
				view.tanHalfAngle = std::tan(camera.GetViewAngle() * 0.5f);
				view.aspect = camera.GetAspect();
				view.isOrtho = isOrtho;
				return view;
			}

			/** その奥行き（Z）で見えている範囲（少し見下ろしているので、縦は少し広めにする） */
			math::Rect CalcVisibleRect(const float z) const
			{
				const float forwardZ = math::Max(forward.z, MIN_FORWARD_Z);
				const float distance = (z - eye.z) / forwardZ;
				const Vector3 center = eye + forward * distance;
				const math::Vec2 half = isOrtho
					? halfSize
					: math::Vec2(tanHalfAngle * distance * aspect, tanHalfAngle * distance);
				const math::Vec2 margined(half.x + VIEW_MARGIN, half.y / forwardZ + VIEW_MARGIN);
				return Expand({ math::Vec2(center.x, center.y), math::Vec2(center.x, center.y) }, margined);
			}

			/**
			 * @brief 画面で見て、地形の面（Z = 0）のどこに重なって見えるか
			 * @details その点を見る線が、地形の面を通る所（シェーダーの ClipOutsideStage と同じ）
			 */
			math::Vec2 ProjectToStage(const Vector3& point) const
			{
				Vector3 direction = forward;
				if (!isOrtho)
				{
					direction = point - eye;
					direction.Normalize();
				}
				const float t = point.z / math::Max(direction.z, MIN_FORWARD_Z);
				return math::Vec2(point.x - direction.x * t, point.y - direction.y * t);
			}

			/** シェーダーに渡す形（xyz = 向き, w = 平行投影なら 1） */
			Vector4 ToShaderValue() const
			{
				return Vector4(forward.x, forward.y, forward.z, isOrtho ? 1.0f : 0.0f);
			}
		};


		void BackgroundRenderer::Init(const BackgroundStyle& style)
		{
			m_style = style;
			m_initialStyle = style;
			InitLayer(m_near);
			InitLayer(m_far);

			m_wallConstants = LayerConstants();
			m_wall.Init({ BOX_MODEL_PATH, BACKGROUND_SHADER_PATH, STONE_PS_ENTRY, enModelUpAxisZ, &m_wallConstants, sizeof(LayerConstants) });
			m_backdropConstants = LayerConstants();
			m_backdrop.Init({ BOX_MODEL_PATH, BACKGROUND_SHADER_PATH, BACKDROP_PS_ENTRY, enModelUpAxisZ, &m_backdropConstants, sizeof(LayerConstants) });

			m_torchLights.clear();
			m_hasLayout = false;
			m_isPlaced = false;
			m_torchLightCount = 0;
			m_time = 0.0f;
			m_isInited = true;
		}


		void BackgroundRenderer::InitLayer(Layer& layer)
		{
			layer.constants = LayerConstants();
			layer.flameConstants = FlameConstants();
			layer.bays.Init({ BAY_MODEL_PATH, BACKGROUND_SHADER_PATH, STONE_PS_ENTRY, enModelUpAxisY, &layer.constants, sizeof(LayerConstants) });
			layer.brokenBays.Init({ BROKEN_BAY_MODEL_PATH, BACKGROUND_SHADER_PATH, STONE_PS_ENTRY, enModelUpAxisY, &layer.constants, sizeof(LayerConstants) });
			layer.torches.Init({ TORCH_MODEL_PATH, BACKGROUND_SHADER_PATH, PROP_PS_ENTRY, enModelUpAxisY, &layer.constants, sizeof(LayerConstants) });
			layer.flames.Init({ FLAME_MODEL_PATH, FLAME_SHADER_PATH, FLAME_PS_ENTRY, enModelUpAxisY, &layer.flameConstants, sizeof(FlameConstants) });
			layer.layout = BackgroundLayerLayout();
			layer.layoutSetting = BackgroundLayoutSetting();
		}


		void BackgroundRenderer::Update(const Stage& stage, const Camera& camera, const bool isOrtho, const float deltaTime)
		{
			if (!m_isInited)
			{
				return;
			}
			m_time += deltaTime;

			// 前のフレームに置いた物を片付ける
			m_near.bays.Clear();
			m_near.brokenBays.Clear();
			m_near.torches.Clear();
			m_near.flames.Clear();
			m_far.bays.Clear();
			m_far.brokenBays.Clear();
			m_far.torches.Clear();
			m_far.flames.Clear();
			m_wall.Clear();
			m_backdrop.Clear();
			m_torchLights.clear();
			m_torchLightCount = 0;

			// 地形が1つもなければ、範囲がわからないので何も置かない
			math::Rect bounds;
			m_isPlaced = camera::CalcStageBounds(stage, bounds);
			if (!m_isPlaced)
			{
				return;
			}
			RebuildIfChanged(bounds);

			const View view = View::FromCamera(camera, isOrtho);
			PlaceBackdrop(view);
			if (m_style.isVisible)
			{
				PlaceWall(view);
				PlaceLayer(m_far, m_style.farLayer, m_style.farTorch, stage, view);
				PlaceLayer(m_near, m_style.nearLayer, m_style.nearTorch, stage, view);
			}
			ApplyTorchLights();
		}


		void BackgroundRenderer::Draw(RenderContext& rc)
		{
			if (!m_isInited || !m_isPlaced)
			{
				return;
			}
			// 自分の定数バッファを持っているので、ライトの値（カメラの位置など）をそれぞれに写す
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			m_backdropConstants.light = light;
			m_wallConstants.light = light;
			for (Layer* layer : { &m_far, &m_near })
			{
				layer->constants.light = light;
				layer->flameConstants.light = light;
			}

			// 奥から順に登録する（不透明なので順番は見た目に関係ないが、奥から描くほうがわかりやすい）
			m_backdrop.Draw();
			m_wall.Draw();
			for (Layer* layer : { &m_far, &m_near })
			{
				layer->bays.Draw();
				layer->brokenBays.Draw();
				layer->torches.Draw();
				layer->flames.Draw();
			}
		}


		size_t BackgroundRenderer::GetModelCount() const
		{
			size_t count = m_wall.GetCount() + m_backdrop.GetCount();
			for (const Layer* layer : { &m_far, &m_near })
			{
				count += layer->bays.GetCount() + layer->brokenBays.GetCount() + layer->torches.GetCount() + layer->flames.GetCount();
			}
			return count;
		}


		void BackgroundRenderer::RebuildIfChanged(const math::Rect& bounds)
		{
			const bool isBoundsChanged = !m_hasLayout || !IsSameRect(bounds, m_bounds);
			m_bounds = bounds;
			m_hasLayout = true;

			const BackgroundLayoutSetting nearSetting = MakeLayoutSetting(m_style.nearLayer, m_style.nearTorch, 0u);
			if (isBoundsChanged || nearSetting != m_near.layoutSetting)
			{
				m_near.layout = BackgroundLayerLayout::Build(bounds, nearSetting);
				m_near.layoutSetting = nearSetting;
			}
			const BackgroundLayoutSetting farSetting = MakeLayoutSetting(m_style.farLayer, m_style.farTorch, FAR_SEED_OFFSET);
			if (isBoundsChanged || farSetting != m_far.layoutSetting)
			{
				m_far.layout = BackgroundLayerLayout::Build(bounds, farSetting);
				m_far.layoutSetting = farSetting;
			}
		}


		BackgroundLayoutSetting BackgroundRenderer::MakeLayoutSetting(const BackgroundLayerStyle& style, const BackgroundTorchStyle& torch, const std::uint32_t seedOffset) const
		{
			BackgroundLayoutSetting setting;
			setting.scale = math::Max(style.scale, MIN_SCALE);
			setting.parallax = style.parallax;
			setting.torchInterval = torch.pillarInterval;
			setting.torchHeight = torch.height;
			setting.brokenRate = m_style.brokenRate;
			setting.seed = static_cast<std::uint32_t>(m_style.seed) + seedOffset;
			return setting;
		}


		void BackgroundRenderer::PlaceBackdrop(const View& view)
		{
			// カメラの見ている所より広い板を、いちばん奥に置く（Box.tkm は原点が底面の真ん中・Z-up）
			const math::Rect visible = view.CalcVisibleRect(BACKDROP_Z);
			const math::Vec2 size(visible.Width() * BACKDROP_EXPAND, visible.Height() * BACKDROP_EXPAND);
			const math::Vec2 center = CenterOf(visible);
			m_backdrop.Place(
				Vector3(center.x, center.y - size.y * 0.5f, BACKDROP_Z + WALL_DEPTH * 0.5f),
				Vector3(size.x / BOX_MODEL_SIZE, size.y / BOX_MODEL_SIZE, WALL_DEPTH / BOX_MODEL_SIZE));

			ShaderValues& values = m_backdropConstants.values;
			values = ShaderValues();
			values.tint = ToColor(m_style.outsideColor);
			values.view = view.ToShaderValue();
		}


		void BackgroundRenderer::PlaceWall(const View& view)
		{
			const BackgroundLayerStyle& style = m_style.wall;
			const math::Vec2 offset = CalcParallaxOffset(style.parallax, view);
			WriteLayerValues(m_wallConstants.values, style, offset, 1.0f, view);
			if (!style.isVisible)
			{
				return;
			}
			// ステージの範囲を、カメラについてくる分と余白だけ広げた壁を1枚置く
			const float parallax = math::Clamp01(style.parallax);
			const math::Vec2 expand(m_bounds.Width() * 0.5f * parallax + WALL_MARGIN, m_bounds.Height() * 0.5f * parallax + WALL_MARGIN);
			const math::Rect area = Expand(m_bounds, expand);
			const math::Vec2 center = CenterOf(area) + offset;
			m_wall.Place(
				Vector3(center.x, area.min.y + offset.y, style.z + WALL_DEPTH * 0.5f),
				Vector3(area.Width() / BOX_MODEL_SIZE, area.Height() / BOX_MODEL_SIZE, WALL_DEPTH / BOX_MODEL_SIZE));
		}


		void BackgroundRenderer::PlaceLayer(Layer& layer, const BackgroundLayerStyle& style, const BackgroundTorchStyle& torch, const Stage& stage, const View& view)
		{
			const float scale = math::Max(style.scale, MIN_SCALE);
			const math::Vec2 offset = CalcParallaxOffset(style.parallax, view);
			WriteLayerValues(layer.constants.values, style, offset, scale, view);
			layer.flameConstants.color = torch.flameOuter.ToShaderValue();
			layer.flameConstants.coreColor = torch.flameInner.ToShaderValue();
			if (!style.isVisible)
			{
				return;
			}

			// ---- 区画（見えている所だけ） ----
			const math::Rect visible = view.CalcVisibleRect(style.z);
			const float bayWidth = background_shape::BAY_WIDTH * scale;
			const float tierHeight = background_shape::TIER_HEIGHT * scale;
			const Vector3 modelScale(scale, scale, scale);
			for (const BackgroundBay& bay : layer.layout.GetBays())
			{
				// 区画は左の柱の足元から右へ。柱の台座が左に少しはみ出すので、そのぶん広く見る
				const math::Vec2 foot = bay.foot + offset;
				const math::Rect rect{ { foot.x - bayWidth * 0.15f, foot.y }, { foot.x + bayWidth, foot.y + tierHeight } };
				if (!rect.Overlaps(visible))
				{
					continue;
				}
				BackgroundModelPool& pool = bay.kind == BackgroundBayKind::Broken ? layer.brokenBays : layer.bays;
				pool.Place(Vector3(foot.x, foot.y, style.z), modelScale);
			}

			// ---- 松明（金具・炎・光の候補） ----
			if (!torch.isVisible)
			{
				return;
			}
			const float range = torch.lightRange * scale;
			// 画面の少し外の松明の光も、画面の中を照らすので候補にする
			const math::Rect lightArea = Expand(visible, math::Vec2(range, range));
			const float z = style.z - background_shape::TORCH_DEPTH * scale;
			for (const BackgroundTorch& placed : layer.layout.GetTorches())
			{
				const math::Vec2 root = placed.root + offset;
				const Vector3 rootPosition(root.x, root.y, z);
				if (!Contains(lightArea, root))
				{
					continue;
				}
				// 画面で見て重なっている地形の面の位置で調べる
				const math::Vec2 onStage = view.ProjectToStage(rootPosition);
				// ステージの外の松明は、炎だけ浮いて見えてしまうので置かない
				// 暗がりの中の松明も置かない（明かりがあると暗く見えないので、光も出さない）
				if (!Contains(m_bounds, onStage) || IsInDarkZone(stage, onStage))
				{
					continue;
				}
				const float wave = CalcFlicker(placed.phase, torch);
				if (Contains(visible, root))
				{
					layer.torches.Place(rootPosition, modelScale);
					// 炎のモデルは高さ 1・幅 1。高くなったときは少し細く（炎が伸び縮みしているように見せる）
					const float width = torch.flameWidth * scale * (1.0f - torch.flickerAmount * 0.5f * wave);
					const float height = torch.flameHeight * scale * (1.0f + torch.flickerAmount * wave);
					layer.flames.Place(rootPosition, Vector3(width, height, width));
				}

				TorchLight light;
				light.position = rootPosition + Vector3(0.0f, torch.flameHeight * scale * LIGHT_HEIGHT_RATE, -LIGHT_FRONT * scale);
				light.range = range;
				const float intensity = torch.lightIntensity * (1.0f + torch.flickerAmount * wave);
				light.color = Vector3(torch.lightColor[0] * intensity, torch.lightColor[1] * intensity, torch.lightColor[2] * intensity);
				light.distanceSq = (root - view.target).LengthSq();
				m_torchLights.push_back(light);
			}
		}


		void BackgroundRenderer::ApplyTorchLights()
		{
			// 注視点に近い順に MAX_TORCH_LIGHT 個まで
			// （windows.h の min マクロとぶつからないよう、std::min は使わない）
			const size_t maxCount = static_cast<size_t>(MAX_TORCH_LIGHT);
			const size_t count = m_torchLights.size() < maxCount ? m_torchLights.size() : maxCount;
			std::partial_sort(m_torchLights.begin(), m_torchLights.begin() + count, m_torchLights.end(),
				[](const TorchLight& a, const TorchLight& b) { return a.distanceSq < b.distanceSq; });
			m_torchLightCount = static_cast<int>(count);

			// 光はどの層も同じ物を使う（手前の松明の光が奥の壁も少し照らす）
			for (ShaderValues* values : { &m_near.constants.values, &m_far.constants.values, &m_wallConstants.values })
			{
				values->torchCount = Vector4(static_cast<float>(count), 0.0f, 0.0f, 0.0f);
				for (size_t i = 0; i < count; ++i)
				{
					const TorchLight& light = m_torchLights[i];
					values->torchPositionRange[i] = Vector4(light.position.x, light.position.y, light.position.z, light.range);
					values->torchColor[i] = Vector4(light.color.x, light.color.y, light.color.z, 0.0f);
				}
			}
		}


		void BackgroundRenderer::WriteLayerValues(ShaderValues& values, const BackgroundLayerStyle& style, const math::Vec2& offset, const float textureScale, const View& view) const
		{
			values.layer = Vector4(style.ambient, style.sunLight, math::Clamp01(style.fog), m_style.stoneTextureWorldSize * textureScale);
			values.offset = Vector4(offset.x, offset.y, 0.0f, 0.0f);
			values.tint = ToColor(style.tint);
			values.fogColor = ToColor(m_style.fogColor);
			values.stageBounds = Vector4(m_bounds.min.x, m_bounds.min.y, m_bounds.max.x, m_bounds.max.y);
			values.view = view.ToShaderValue();
		}


		math::Vec2 BackgroundRenderer::CalcParallaxOffset(const float parallax, const View& view) const
		{
			// 透視投影では奥の物は自然にゆっくり動くので、ずらさない
			if (!view.isOrtho)
			{
				return math::Vec2::Zero();
			}
			// ステージの真ん中を見ているときにずれが 0 になるようにする（並べ方の範囲もこれを前提にしている）
			return (view.target - CenterOf(m_bounds)) * math::Clamp01(parallax);
		}


		float BackgroundRenderer::CalcFlicker(const float phase, const BackgroundTorchStyle& torch) const
		{
			// 速さの違う2つの波を足して、同じ動きのくり返しに見えないようにする（ゴールの門の炎と同じ）
			const float t = m_time * torch.flickerSpeed + phase;
			return std::sin(t) * 0.6f + std::sin(t * 2.3f + phase * 1.7f) * 0.4f;
		}
	}
}
