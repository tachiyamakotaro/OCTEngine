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