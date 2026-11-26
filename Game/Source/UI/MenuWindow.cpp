/**
 * @file MenuWindow.cpp
 * @brief メニューのウィンドウ
 */
#include "stdafx.h"
#include "MenuWindow.h"


namespace app
{
	namespace ui
	{
		namespace
		{
			/** 角の丸い四角を描くシェーダー */
			const char* const WINDOW_SHADER_PATH = "Assets/shader/menuWindow.fx";
			/** 板の画像（シェーダーは色を読まないが、Sprite は画像が要るので、フェードの画像を使う） */
			const char* const WINDOW_SPRITE_PATH = "Assets/sprite/fade.dds";

			// ---- 形（ピクセル） ----
			/** 角の丸み */
			constexpr float CORNER_RADIUS = 18.0f;
			/** 枠の太さ */
			constexpr float FRAME_WIDTH = 3.0f;
			/** 影の広がり（板はウィンドウより、この分だけ上下左右に大きい） */
			constexpr float SHADOW_SPREAD = 28.0f;

			// ---- 色（遺跡に合わせた、暗い石の色と金） ----
			/** 地（上の端・下の端。少し透けて、後ろのぼかした背景が見える） */
			const Vector4 FILL_TOP_COLOR(0.10f, 0.085f, 0.07f, 0.86f);
			const Vector4 FILL_BOTTOM_COLOR(0.05f, 0.042f, 0.035f, 0.90f);
			/** 枠と線（金） */
			const Vector4 FRAME_COLOR(0.86f, 0.68f, 0.30f, 1.0f);
			/** 見出しの帯（地より少し明るい、焦げた金） */
			const Vector4 HEADER_COLOR(0.22f, 0.16f, 0.07f, 0.94f);
		}


		MenuWindow::MenuWindow()
			: m_sprite()
			, m_values()
			, m_isInited(false)
		{}


		MenuWindow::~MenuWindow()
		{}


		void MenuWindow::Init(const MenuWindowShape& shape)
		{
			// 影がはみ出す分だけ、板を広げる
			const float quadWidth = shape.size.x + SHADOW_SPREAD * 2.0f;
			const float quadHeight = shape.size.y + SHADOW_SPREAD * 2.0f;
			m_values.size = Vector4(shape.size.x, shape.size.y, quadWidth, quadHeight);
			m_values.shape = Vector4(CORNER_RADIUS, FRAME_WIDTH, shape.headerHeight, SHADOW_SPREAD);
			m_values.fillTop = FILL_TOP_COLOR;
			m_values.fillBottom = FILL_BOTTOM_COLOR;
			m_values.frameColor = FRAME_COLOR;
			m_values.headerColor = HEADER_COLOR;

			SpriteInitData initData;
			initData.m_ddsFilePath[0] = WINDOW_SPRITE_PATH;
			initData.m_fxFilePath = WINDOW_SHADER_PATH;
			initData.m_width = static_cast<UINT>(quadWidth);
			initData.m_height = static_cast<UINT>(quadHeight);
			initData.m_alphaBlendMode = AlphaBlendMode_Trans;
			// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
			initData.m_expandConstantBuffer = &m_values;
			initData.m_expandConstantBufferSize = sizeof(m_values);
			m_sprite.Init(initData);

			// 動かさないので、ここで置いておく
			m_sprite.Update(Vector3(shape.center.x, shape.center.y, 0.0f), Quaternion::Identity, Vector3::One);
			m_isInited = true;
		}


		void MenuWindow::Draw(RenderContext& rc)
		{
			(void)rc;
			if (!m_isInited)
			{
				return;
			}
			// SpriteRender と同じく、描く予約だけする（RenderingEngine が 3D の後に、予約した順に描く）
			RenderingEngine::GetInstance()->AddRenderObject2D(m_sprite);
		}
	}
}
