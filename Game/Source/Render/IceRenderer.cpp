/**
 * @file IceRenderer.cpp
 * @brief 氷を、穴の縁がなめらかに丸く消えていくように描く
 */
#include "stdafx.h"
#include "IceRenderer.h"
#include "MirrorReflection.h"
#include "Source/Core/Math/MathUtil.h"
#include "Source/Stage/Stage.h"


namespace
{
	/** シェーダー */
	const char* const SHADER_PATH = "Assets/shader/ice.fx";
	/** ピクセルシェーダー（残り具合が小さい所を捨て、穴の縁を明るくする） */
	const char* const PS_ENTRY = "PSMainIce";
	/** 氷の残り具合を渡す拡張 SRV の番号（0 はシャドウマップ。番号 + 10 がシェーダーのレジスタ → t11） */
	constexpr int FIELD_SRV_SLOT = 1;
	/** 「溶けている最中」のテクスチャを渡す拡張 SRV の番号（横と下の面 → t12、上の面 → t13） */
	constexpr int WARM_SIDE_SRV_SLOT = 2;
	constexpr int WARM_TOP_SRV_SLOT = 3;


	/** "〜.tkm" → "〜_<face>.dds"（仮モデルのテクスチャの名前） */
	std::wstring MakeTexturePath(const std::string& modelPath, const int face)
	{
		std::string path = modelPath;
		const size_t dot = path.rfind('.');
		if (dot != std::string::npos)
		{
			path.erase(dot);
		}
		path += face == 0 ? "_0.dds" : "_1.dds";
		// パスは英数字だけなので、そのまま wchar_t にする
		return std::wstring(path.begin(), path.end());
	}


	void Set4(float out[4], const float x, const float y, const float z, const float w)
	{
		out[0] = x;
		out[1] = y;
		out[2] = z;
		out[3] = w;
	}
}


namespace app
{
	namespace render
	{
		IceRenderer::IceRenderer()
			: m_config()
			, m_views()
			, m_warmTextures()
			, m_isWarmTextureTried()
			, m_viewCount(0)
			, m_isInited(false)
		{}


		void IceRenderer::Init(const Config& config)
		{
			m_config = config;
			m_isInited = true;
		}


		void IceRenderer::Update(const stage::Stage& stage)
		{
			m_viewCount = 0;
			if (!m_isInited)
			{
				return;
			}

			const stage::BackMirror& backMirror = stage.GetBackMirror();
			const bool isMirrorEnabled = backMirror.isEnabled && m_config.isDrawReflection;
			const float unit = math::Max(m_config.modelSize, 0.01f);

			for (const stage::IceBlock& ice : stage.GetGimmicks().View<stage::IceBlock>())
			{
				IceView& view = AcquireView(m_viewCount, ice);
				++m_viewCount;
				view.isVisible = !ice.IsMelted();
				if (!view.isVisible)
				{
					continue;
				}
				view.warmth = math::Clamp01(ice.warmth);
				view.isReflected = isMirrorEnabled;
				// マスごとの残り具合（GPU に送るのは Draw のとき）
				ice.BuildSmoothField(view.field);

				// 箱を氷の元の大きさに合わせて置く（穴の形はシェーダーで切り抜く）
				const math::Vec2 center = (ice.rect.min + ice.rect.max) * 0.5f;
				const Vector3 position(center.x, center.y, 0.0f);
				const Vector3 scale(ice.rect.Width() / unit, ice.rect.Height() / unit, m_config.depth / unit);
				Quaternion rotation;
				rotation.SetRotationDegZ(0.0f);

				const bool hasWarmTexture = AcquireWarmTexture(view.isKeep, 0) != nullptr && AcquireWarmTexture(view.isKeep, 1) != nullptr;
				IceModel& model = AcquireModel(view, false);
				SetConstants(model.constants, ice, view, false, hasWarmTexture);
				model.model.UpdateWorldMatrix(position, rotation, scale);

				if (view.isReflected)
				{
					IceModel& reflected = AcquireModel(view, true);
					SetConstants(reflected.constants, ice, view, true, hasWarmTexture);
					reflected.model.UpdateWorldMatrix(
						ReflectPositionZ(position, backMirror.planeZ),
						ReflectRotationZ(rotation),
						ReflectScaleZ(scale));
				}
			}
		}


		void IceRenderer::Draw(RenderContext& rc)
		{
			(void)rc;
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			for (size_t i = 0; i < m_viewCount; ++i)
			{
				IceView& view = *m_views[i];
				if (!view.isVisible)
				{
					continue;
				}
				// 氷の残り具合を GPU に送る（構造化バッファは今のバックバッファの分だけ書き換わる）
				view.fieldBuffer.Update(view.field.data());

				IceModel* model = view.models[0].get();
				if (model != nullptr)
				{
					model->constants.light = light;
					RenderingEngine::GetInstance()->AddRenderObject(model->model);
				}
				IceModel* reflected = view.isReflected ? view.models[1].get() : nullptr;
				if (reflected != nullptr)
				{
					reflected->constants.light = light;
					RenderingEngine::GetInstance()->AddRenderObject(reflected->model);
				}
			}
		}


		size_t IceRenderer::GetDrawCount() const
		{
			size_t count = 0;
			for (size_t i = 0; i < m_viewCount; ++i)
			{
				if (m_views[i]->isVisible)
				{
					count += m_views[i]->isReflected ? 2 : 1;
				}
			}
			return count;
		}


		IceRenderer::IceView& IceRenderer::AcquireView(const size_t index, const stage::IceBlock& ice)
		{
			// マスの数が変わると構造化バッファの大きさが変わるので、入れ物ごと作り直す
			// （ふつうは最初の1回だけ。エディタで氷の大きさを変えたときや、ステージを読み直したとき）
			const int columns = ice.GetColumnCount() > 0 ? ice.GetColumnCount() : 1;
			const int rows = ice.GetRowCount() > 0 ? ice.GetRowCount() : 1;
			const bool isKeep = ice.kind == stage::IceKind::Keep;
			if (index < m_views.size())
			{
				IceView& old = *m_views[index];
				if (old.columns == columns && old.rows == rows && old.isKeep == isKeep)
				{
					return old;
				}
			}

			auto view = std::make_unique<IceView>();
			view->columns = columns;
			view->rows = rows;
			view->isKeep = isKeep;
			view->field.assign(static_cast<size_t>(columns * rows), 1.0f);
			view->fieldBuffer.Init(sizeof(float), columns * rows, view->field.data());

			if (index < m_views.size())
			{
				// 古い入れ物の GPU の資源は、エンジンが1フレーム待ってから消す
				m_views[index] = std::move(view);
			}
			else
			{
				m_views.push_back(std::move(view));
			}
			return *m_views[index];
		}


		IceRenderer::IceModel& IceRenderer::AcquireModel(IceView& view, const bool isReflected)
		{
			std::unique_ptr<IceModel>& slot = view.models[isReflected ? 1 : 0];
			if (slot != nullptr)
			{
				return *slot;
			}
			// Model の初期化は重いので、必要になったときに1回だけ作って使い回す
			auto model = std::make_unique<IceModel>();
			model->constants = Constants();

			// モデルはふつうの氷。「溶けている最中」の色は、テクスチャを混ぜてシェーダーで出す
			const std::string& path = view.isKeep ? m_config.keepModelPath : m_config.meltModelPath;
			ModelInitData initData;
			initData.m_tkmFilePath = path.c_str();
			initData.m_fxFilePath = SHADER_PATH;
			initData.m_vsEntryPointFunc = "VSMain";
			initData.m_vsSkinEntryPointFunc = "VSMain";
			initData.m_psEntryPointFunc = PS_ENTRY;
			initData.m_modelUpAxis = enModelUpAxisY;
			// 穴の向こうに箱の内側（裏の面）が見えるように、両面を描く
			initData.m_cullMode = D3D12_CULL_MODE_NONE;
			// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
			initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
			initData.m_expandConstantBuffer = &model->constants;
			initData.m_expandConstantBufferSize = sizeof(Constants);
			// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
			initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
			// 氷の残り具合（t11）
			initData.m_expandShaderResoruceView[FIELD_SRV_SLOT] = &view.fieldBuffer;
			// 溶けている最中の色のテクスチャ（t12 = 横と下の面、t13 = 上の面）。読めなかったときは、つなぐだけのためにシャドウマップを入れておく（使わない）
			Texture* warmSide = AcquireWarmTexture(view.isKeep, 0);
			Texture* warmTop = AcquireWarmTexture(view.isKeep, 1);
			const bool hasWarmTexture = warmSide != nullptr && warmTop != nullptr;
			initData.m_expandShaderResoruceView[WARM_SIDE_SRV_SLOT] = hasWarmTexture ? warmSide : &RenderingEngine::GetInstance()->GetShadowMapTexture();
			initData.m_expandShaderResoruceView[WARM_TOP_SRV_SLOT] = hasWarmTexture ? warmTop : &RenderingEngine::GetInstance()->GetShadowMapTexture();
			model->model.Init(initData);

			slot = std::move(model);
			return *slot;
		}


		Texture* IceRenderer::AcquireWarmTexture(const bool isKeep, const int face)
		{
			const int kind = isKeep ? 1 : 0;
			const int side = face == 0 ? 0 : 1;
			if (!m_isWarmTextureTried[kind][side])
			{
				m_isWarmTextureTried[kind][side] = true;
				const std::string& modelPath = isKeep ? m_config.keepLitModelPath : m_config.meltLitModelPath;
				const std::wstring texturePath = MakeTexturePath(modelPath, side);
				auto texture = std::make_unique<Texture>();
				texture->InitFromDDSFile(texturePath.c_str());
				if (texture->IsValid())
				{
					m_warmTextures[kind][side] = std::move(texture);
				}
			}
			return m_warmTextures[kind][side].get();
		}


		void IceRenderer::SetConstants(Constants& constants, const stage::IceBlock& ice, const IceView& view, const bool isReflected, const bool hasWarmTexture) const
		{
			const Config& c = m_config;
			Set4(constants.iceRect, ice.rect.min.x, ice.rect.min.y, ice.rect.max.x, ice.rect.max.y);
			Set4(constants.iceGrid,
				static_cast<float>(view.columns), static_cast<float>(view.rows),
				ice.rect.Width() / static_cast<float>(view.columns), ice.rect.Height() / static_cast<float>(view.rows));
			Set4(constants.iceParam, math::Clamp01(c.threshold), math::Max(c.edgeWidth, 0.0001f), math::Clamp01(c.innerShade), isReflected ? 1.0f : 0.0f);
			// 縁の色も、温まり具合で「冷えているとき」と「溶けている最中」の色を混ぜる
			const float w = view.warmth;
			Set4(constants.iceEdgeColor,
				math::Lerp(c.edgeColor[0], c.meltingEdgeColor[0], w),
				math::Lerp(c.edgeColor[1], c.meltingEdgeColor[1], w),
				math::Lerp(c.edgeColor[2], c.meltingEdgeColor[2], w),
				math::Clamp01(math::Lerp(c.edgeColor[3], c.meltingEdgeColor[3], w)));
			Set4(constants.iceWarm, w, hasWarmTexture ? 1.0f : 0.0f, 0.0f, 0.0f);
		}
	}
}
