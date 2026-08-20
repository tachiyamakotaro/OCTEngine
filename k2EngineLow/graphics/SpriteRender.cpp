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

		m_sprite.Init(initData);
	}

	void SpriteRender::Update()
	{
		m_sprite.Update(m_position, m_rotation, m_scale);
	}

	void SpriteRender::Draw(RenderContext& rc)
	{
		m_sprite.Draw(rc);
	}
}