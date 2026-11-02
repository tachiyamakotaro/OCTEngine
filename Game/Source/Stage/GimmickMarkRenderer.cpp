/**
 * @file GimmickMarkRenderer.cpp
 * @brief ギミックに「何ができるか」の印を描く（受光器の電球マーク・回せる反射鏡の歯車・動かせる反射鏡のレール）
 */
#include "stdafx.h"
#include "GimmickMarkRenderer.h"

#include "Stage.h"


namespace
{
	/** 印の板に使うモデル（白いテクスチャの箱。Box.tkm と同じ形・大きさ・原点） */
	const char* MODEL_PATH = "Assets/modelData/solar_panel.tkm";
	/** モデルの元の大きさ（1辺） */
	constexpr float MODEL_SIZE = 100.0f;
	/** シェーダーのファイル */
	const char* SHADER_PATH = "Assets/shader/gimmickMark.fx";
	/** ピクセルシェーダーの関数名 */
	const char* PS_ENTRY = "PSMainGimmickMark";
	/** 電球マークの板の厚さ */
	constexpr float BULB_THICKNESS = 2.0f;
	/** 止め金の左右（MakeStopper の side） */
	constexpr float LEFT_SIDE = -1.0f;
	constexpr float RIGHT_SIDE = 1.0f;
}


namespace app
{
	namespace stage
	{
		void GimmickMarkRenderer::Init(const GimmickMarkParameter& parameter)
		{
			m_parameter = parameter;
			m_initialParameter = parameter;
			m_marks.clear();
			m_count = 0;
			m_isInited = true;
		}


		void GimmickMarkRenderer::Update(const Stage& stage)
		{
			if (!m_isInited)
			{
				return;
			}
			m_count = 0;
			PutBulbs(stage);
			PutMirrorMarks(stage);
		}


		void GimmickMarkRenderer::Draw(RenderContext& rc)
		{
			// 印は1つずつ自分の定数バッファを持っているので、ライトの値（カメラの位置など）もそれぞれに写す
			const SceneLightData& light = SceneLight::GetInstance()->GetSceneLight();
			RenderingEngine* renderingEngine = RenderingEngine::GetInstance();
			for (size_t i = 0; i < m_count; ++i)
			{
				m_marks[i]->constants.light = light;
				renderingEngine->AddRenderObject(m_marks[i]->model);
			}
		}


		void GimmickMarkRenderer::PutBulbs(const Stage& stage)
		{
			if (!m_parameter.bulb.isVisible)
			{
				return;
			}
			for (const LightReceiver& receiver : stage.GetGimmicks().View<LightReceiver>())
			{
				Put(MakeBulb(receiver));
			}
		}


		void GimmickMarkRenderer::PutMirrorMarks(const Stage& stage)
		{
			for (const LightMirror& mirror : stage.GetGimmicks().View<LightMirror>())
			{
				// 動かせる範囲が決まっている鏡：レールと両端の止め金
				if (m_parameter.rail.isVisible && mirror.HasMoveRange())
				{
					const LightMirror::MoveRange range = mirror.GetMoveRange();
					Put(MakeRail(range));
					Put(MakeStopper(range, LEFT_SIDE));
					Put(MakeStopper(range, RIGHT_SIDE));
				}
				// 回せる鏡：回る軸に歯車（レールより手前に来る）
				if (m_parameter.gear.isVisible && mirror.isRotatable)
				{
					Put(MakeGear(mirror));
				}
			}
		}


		GimmickMarkRenderer::MarkPlate GimmickMarkRenderer::MakeBulb(const LightReceiver& receiver) const
		{
			const BulbStyle& style = m_parameter.bulb;
			const math::Rect& rect = receiver.rect;
			const float iconSize = math::Max(math::Min(rect.Width(), rect.Height()) * style.scale, 0.0f);
			// 受光器の手前の面（Z = -奥行きの半分）のすぐ手前に、薄い板を置く
			const float z = -m_parameter.placement.receiverDepth * 0.5f - m_parameter.placement.surfaceGap - BULB_THICKNESS * 0.5f;

			MarkPlate plate;
			plate.center = Vector3((rect.min.x + rect.max.x) * 0.5f, (rect.min.y + rect.max.y) * 0.5f, z);
			plate.size = Vector3(iconSize, iconSize, BULB_THICKNESS);
			plate.values.color = style.off.ToShaderValue();
			plate.values.darkColor = Vector4(style.outline.rgb[0], style.outline.rgb[1], style.outline.rgb[2], static_cast<float>(Shape::Bulb));
			plate.values.center = Vector4(plate.center.x, plate.center.y, plate.center.z, iconSize * 0.5f);
			// y = 光り具合（0 = 消えている 〜 1 = 光っている）
			plate.values.param = Vector4(0.0f, receiver.isLit ? 1.0f : 0.0f, 0.0f, 0.0f);
			plate.values.litColor = style.lit.ToShaderValue();
			return plate;
		}


		GimmickMarkRenderer::MarkPlate GimmickMarkRenderer::MakeGear(const LightMirror& mirror) const
		{
			const GearStyle& style = m_parameter.gear;

			MarkPlate plate;
			plate.center = Vector3(mirror.center.x, mirror.center.y, GetGearZ());
			plate.size = Vector3(style.size, style.size, style.thickness);
			plate.values.color = style.body.ToShaderValue();
			plate.values.darkColor = Vector4(style.dark.rgb[0], style.dark.rgb[1], style.dark.rgb[2], static_cast<float>(Shape::Gear));
			plate.values.center = Vector4(plate.center.x, plate.center.y, plate.center.z, style.size * 0.5f);
			// x = 回転（鏡の角度。ラジアン）, w = 歯の数
			plate.values.param = Vector4(math::DegToRad(mirror.angleDeg), 0.0f, 0.0f, style.teeth);
			plate.values.litColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			return plate;
		}


		GimmickMarkRenderer::MarkPlate GimmickMarkRenderer::MakeRail(const LightMirror::MoveRange& range) const
		{
			const RailStyle& style = m_parameter.rail;
			const float halfLength = range.Length() * 0.5f + math::Max(style.extend, 0.0f);

			MarkPlate plate;
			plate.center = Vector3(range.CenterX(), range.y, GetRailZ());
			plate.size = Vector3(halfLength * 2.0f, style.height, style.depth);
			plate.values.color = style.body.ToShaderValue();
			plate.values.darkColor = Vector4(style.dark.rgb[0], style.dark.rgb[1], style.dark.rgb[2], static_cast<float>(Shape::Rail));
			plate.values.center = Vector4(plate.center.x, plate.center.y, plate.center.z, style.height * 0.5f);
			// z = 長さの半分, w = 枕木の間隔
			plate.values.param = Vector4(0.0f, 0.0f, halfLength, style.tieSpacing);
			plate.values.litColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			return plate;
		}


		GimmickMarkRenderer::MarkPlate GimmickMarkRenderer::MakeStopper(const LightMirror::MoveRange& range, const float side) const
		{
			const RailStyle& style = m_parameter.rail;
			// レールの端のすぐ外側（鏡の中心はここで止まる）
			const float halfLength = range.Length() * 0.5f + math::Max(style.extend, 0.0f);
			const float x = range.CenterX() + side * (halfLength + style.stopperWidth * 0.5f);

			MarkPlate plate;
			plate.center = Vector3(x, range.y, GetRailZ());
			plate.size = Vector3(style.stopperWidth, style.stopperHeight, style.depth);
			plate.values.color = style.stopper.ToShaderValue();
			plate.values.darkColor = Vector4(0.0f, 0.0f, 0.0f, static_cast<float>(Shape::Solid));
			plate.values.center = Vector4(plate.center.x, plate.center.y, plate.center.z, style.stopperHeight * 0.5f);
			plate.values.param = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			plate.values.litColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			return plate;
		}


		void GimmickMarkRenderer::Put(const MarkPlate& plate)
		{
			// 大きさがない板は置かない
			if (plate.size.x <= 0.0f || plate.size.y <= 0.0f || plate.size.z <= 0.0f)
			{
				return;
			}
			// モデルの原点は底面の中心なので、中心から高さの半分だけ下げる
			const Vector3 position(plate.center.x, plate.center.y - plate.size.y * 0.5f, plate.center.z);
			const Vector3 scale(plate.size.x / MODEL_SIZE, plate.size.y / MODEL_SIZE, plate.size.z / MODEL_SIZE);

			MarkModel& mark = Acquire();
			mark.constants.mark = plate.values;
			mark.model.UpdateWorldMatrix(position, Quaternion::Identity, scale);
		}


		GimmickMarkRenderer::MarkModel& GimmickMarkRenderer::Acquire()
		{
			// Model の初期化は重いので、足りなくなったときだけ増やし、毎フレーム使い回す
			if (m_count >= m_marks.size())
			{
				auto mark = std::make_unique<MarkModel>();

				ModelInitData initData;
				initData.m_tkmFilePath = MODEL_PATH;
				initData.m_fxFilePath = SHADER_PATH;
				initData.m_vsEntryPointFunc = "VSMain";
				initData.m_vsSkinEntryPointFunc = "VSMain";
				initData.m_psEntryPointFunc = PS_ENTRY;
				// Box.tkm と同じ Z-up
				initData.m_modelUpAxis = enModelUpAxisZ;
				initData.m_cullMode = D3D12_CULL_MODE_BACK;
				// メインのレンダーターゲット（RenderingEngine の m_mainRenderTarget）と同じ形式
				initData.m_colorBufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
				// シェーダーに渡す値（b1）。Draw のたびに中身が GPU に送られる
				initData.m_expandConstantBuffer = &mark->constants;
				initData.m_expandConstantBufferSize = sizeof(Constants);
				// model.fx は t10 のシャドウマップを使うので、ModelRender と同じくつないでおく
				initData.m_expandShaderResoruceView[0] = &RenderingEngine::GetInstance()->GetShadowMapTexture();
				mark->model.Init(initData);

				m_marks.push_back(std::move(mark));
			}
			return *m_marks[m_count++];
		}


		float GimmickMarkRenderer::GetGearZ() const
		{
			// 鏡の手前の面（Z = -奥行きの半分）のすぐ手前に置く（遠くからでも見えるように）
			const MarkPlacement& placement = m_parameter.placement;
			return -(placement.lightMirrorDepth * 0.5f + placement.surfaceGap + m_parameter.gear.thickness * 0.5f);
		}


		float GimmickMarkRenderer::GetRailZ() const
		{
			// 鏡の奥の面のすぐ奥（壁に付いたレールの上を鏡が動いて見えるように）
			const MarkPlacement& placement = m_parameter.placement;
			return placement.lightMirrorDepth * 0.5f + placement.surfaceGap + m_parameter.rail.depth * 0.5f;
		}
	}
}
