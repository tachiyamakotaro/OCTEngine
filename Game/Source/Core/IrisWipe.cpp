/**
 * @file IrisWipe.cpp
 * @brief 丸く閉じるフェード
 */
#include "stdafx.h"
#include "IrisWipe.h"
#include "Source/Core/Math/MathUtil.h"
#include <cmath>


namespace app
{
	namespace core
	{
		namespace
		{
			/** 黒い画像（SceneManager のフェード core::Fade と同じ画像。真っ暗になったときに見た目がつながる） */
			const char* const IRIS_SPRITE_PATH = "Assets/sprite/fade.dds";
			/** 丸い穴をあけるシェーダー */
			const char* const IRIS_SHADER_PATH = "Assets/shader/irisWipe.fx";
			/** 縁のぼかしの最小値（0 だとシェーダーの smoothstep が使えない） */
			constexpr float MIN_EDGE_SOFTNESS = 0.5f;
		}


		void IrisWipe::Init(const IrisStyle& style)
		{
			m_style = style;
			m_initialStyle = style;
			m_closeRate = 0.0f;

			const float screenW = static_cast<float>(FRAME_BUFFER_W);
			const float screenH = static_cast<float>(FRAME_BUFFER_H);
			m_values.circle = Vector4(screenW * 0.5f, screenH * 0.5f, CalcOpenRadius(Vector2(screenW * 0.5f, screenH * 0.5f)), MIN_EDGE_SOFTNESS);
			m_values.screen = Vector4(screenW, screenH, 0.0f, 0.0f);

			SpriteInitData initData;
			initData.m_ddsFilePath[0] = IRIS_SPRITE_PATH;
			initData.m_fxFilePath = IRIS_SHADER_PATH;
			initData.m_width = FRAME_BUFFER_W;
			initData.m_height = FRAME_BUFFER_H;
			// 穴の中を透明にするので、半透明で重ねる
			initData.m_alphaBlendMode = AlphaBlendMode_Trans;
			// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
			initData.m_expandConstantBuffer = &m_values;
			initData.m_expandConstantBufferSize = sizeof(m_values);
			m_sprite.Init(initData);
			// 画面の真ん中に、画面いっぱいの大きさで置く（動かさない）
			m_sprite.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
			m_isInited = true;
		}


		void IrisWipe::Update(const float closeRate, const Vector3& focus)
		{
			m_closeRate = math::Clamp01(closeRate);
			if (m_closeRate <= 0.0f)
			{
				return;
			}
			const Vector2 center = WorldToPixel(focus + Vector3(0.0f, m_style.focusOffsetY, 0.0f));
			const float softness = math::Max(m_style.edgeSoftness, MIN_EDGE_SOFTNESS);
			// 開ききった半径（画面の角まで届く）から 0 まで小さくする。0 になると縁のぼかしの分も含めて真っ暗
			const float radius = (CalcOpenRadius(center) + softness) * (1.0f - m_closeRate);
			m_values.circle = Vector4(center.x, center.y, radius, softness);
		}


		void IrisWipe::Draw(RenderContext& rc)
		{
			(void)rc;
			if (!m_isInited || m_closeRate <= 0.0f)
			{
				return;
			}
			// SpriteRender と同じく、描く予約だけする（RenderingEngine が 3D の後に描く）
			RenderingEngine::GetInstance()->AddRenderObject2D(m_sprite);
		}


		Vector2 IrisWipe::WorldToPixel(const Vector3& world)
		{
			Vector4 clip(world.x, world.y, world.z, 1.0f);
			g_camera3D->GetViewProjectionMatrix().Apply(clip);
			const float w = clip.w > 0.0001f ? clip.w : 0.0001f;
			// -1〜1 の範囲 → 画面の左上が 0 のピクセル（下が +Y）
			const float x = (clip.x / w * 0.5f + 0.5f) * static_cast<float>(FRAME_BUFFER_W);
			const float y = (0.5f - clip.y / w * 0.5f) * static_cast<float>(FRAME_BUFFER_H);
			return Vector2(x, y);
		}


		float IrisWipe::CalcOpenRadius(const Vector2& center)
		{
			const float screenW = static_cast<float>(FRAME_BUFFER_W);
			const float screenH = static_cast<float>(FRAME_BUFFER_H);
			// 中心が画面の外にあっても、画面の全部が穴の中に入る長さ
			const float farX = math::Max(math::Abs(center.x), math::Abs(screenW - center.x));
			const float farY = math::Max(math::Abs(center.y), math::Abs(screenH - center.y));
			return std::sqrt(farX * farX + farY * farY);
		}
	}
}
