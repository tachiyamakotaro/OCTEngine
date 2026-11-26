#include "k2EngineLowPreCompile.h"

#include "SpriteRender.h"

namespace nsK2EngineLow
{
	void SpriteRender::Init(const char* ddsFilePath, const float width, const float height)
	{
		SpriteInitData initData;
		initData.m_ddsFilePath[0] = ddsFilePath;
		initData.m_fxFilePath = "Assets/shader/sprite.fx";
		initData.m_width = static_cast<UINT>(width);
		initData.m_height = static_cast<UINT>(height);
		// 乗算カラーの透明度で透けるようにする（フェードに必要。不透明な画像はそのまま出る）
		initData.m_alphaBlendMode = AlphaBlendMode_Trans;

		m_sprite.Init(initData);
	}

	void SpriteRender::InitWithOutline(const char* ddsFilePath, const float width, const float height, const SpriteOutline& outline)
	{
		// 縁がはみ出す分だけ、板を上下左右に広げる（広げた所は、縁のほかは透明）
		const UINT quadWidth = static_cast<UINT>(width + outline.thickness * 2.0f);
		const UINT quadHeight = static_cast<UINT>(height + outline.thickness * 2.0f);
		const float safeWidth = width > 0.0f ? width : 1.0f;
		const float safeHeight = height > 0.0f ? height : 1.0f;

		m_outlineValues.color = outline.color;
		m_outlineValues.param = Vector4(
			outline.thickness / safeWidth,
			outline.thickness / safeHeight,
			static_cast<float>(quadWidth) / safeWidth,
			static_cast<float>(quadHeight) / safeHeight
		);

		SpriteInitData initData;
		initData.m_ddsFilePath[0] = ddsFilePath;
		initData.m_fxFilePath = "Assets/shader/spriteOutline.fx";
		initData.m_width = quadWidth;
		initData.m_height = quadHeight;
		// Init と同じく、乗算カラーの透明度で透けるようにする
		initData.m_alphaBlendMode = AlphaBlendMode_Trans;
		// 縁の色と太さ（b1）。Draw のたびに GPU に送られる
		initData.m_expandConstantBuffer = &m_outlineValues;
		initData.m_expandConstantBufferSize = sizeof(m_outlineValues);

		m_sprite.Init(initData);
	}

	void SpriteRender::Update()
	{
		m_sprite.Update(m_position, m_rotation, m_scale, m_pivot);
	}

	void SpriteRender::Draw(RenderContext& rc)
	{
		// ModelRender と同じく、描く予約だけする（RenderingEngine が 3D の後に描く）
		RenderingEngine::GetInstance()->AddRenderObject2D(m_sprite);
	}
}