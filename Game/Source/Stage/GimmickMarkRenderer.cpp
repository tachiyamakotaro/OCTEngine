/**
 * @file GimmickMarkRenderer.cpp
 * @brief ギミックに「何ができるか」の印を描く（受光器の電球マーク・回せる反射鏡の歯車・動かせる反射鏡のレール・チェックポイントの旗）
 */
#include "stdafx.h"
#include "GimmickMarkRenderer.h"

#include "Stage.h"
#include <cmath>


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
	/** 旗の板の厚さ */
	constexpr float FLAG_CLOTH_THICKNESS = 2.0f;
	/** 旗をポールのてっぺんからどれだけ下げて付けるか（先の玉と重ならないように） */
	constexpr float FLAG_CLOTH_TOP_GAP = 4.0f;
	/** 旗ごとに、はためきの始まりをずらす角度（ラジアン。並んだ旗が同じ動きにならないように） */
	constexpr float FLAG_PHASE_SHIFT = 1.7f;
	/** はためきの角度をこの値で戻す（ずっと増えて float の精度が落ちないように。2π の倍数） */
	constexpr float FLAG_PHASE_WRAP = app::math::PI * 2.0f * 64.0f;


	/** 印の色を混ぜる（t = 0 で a、t = 1 で b。光る強さも混ぜる） */
	app::stage::MarkColor LerpMarkColor(const app::stage::MarkColor& a, const app::stage::MarkColor& b, const float t)
	{
		app::stage::MarkColor color;
		for (int i = 0; i < 3; ++i)
		{
			color.rgb[i] = app::math::Lerp(a.rgb[i], b.rgb[i], t);
		}
		color.glow = app::math::Lerp(a.glow, b.glow, t);
		return color;
	}
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
			m_flagMotions.clear();
			m_isInited = true;
		}


		void GimmickMarkRenderer::Update(const Stage& stage, const float deltaTime)
		{
			if (!m_isInited)
			{
				return;
			}
			m_count = 0;
			PutBulbs(stage);
			PutMirrorMarks(stage);
			PutFlags(stage, deltaTime);
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
				// 動かせる範囲が決まっている鏡：その範囲にレールと両端の止め金（範囲は鏡に答えてもらう）
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


		void GimmickMarkRenderer::PutFlags(const Stage& stage, const float deltaTime)
		{
			const auto checkpoints = stage.GetGimmicks().View<Checkpoint>();
			// チェックポイントが増えたら、増えた分の動きを作る（はためきの始まりを少しずつずらす）
			while (m_flagMotions.size() < checkpoints.size())
			{
				FlagMotion motion;
				motion.phase = FLAG_PHASE_SHIFT * static_cast<float>(m_flagMotions.size());
				m_flagMotions.push_back(motion);
			}

			const FlagStyle& style = m_parameter.flag;
			size_t index = 0;
			for (const Checkpoint& checkpoint : checkpoints)
			{
				FlagMotion& motion = m_flagMotions[index++];
				motion.Advance(style, checkpoint.isReached, deltaTime);
				if (!style.isVisible)
				{
					continue;
				}
				const Vector3 foot = GetFlagFoot(checkpoint);
				Put(MakeFlagBase(foot));
				Put(MakeFlagPole(foot));
				Put(MakeFlagCloth(foot, motion));
				// 玉は旗とポールの上にかぶせる（最後に置く）
				Put(MakeFlagKnob(foot));
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


		GimmickMarkRenderer::MarkPlate GimmickMarkRenderer::MakeFlagBase(const Vector3& foot) const
		{
			const FlagStyle& style = m_parameter.flag;

			MarkPlate plate;
			plate.center = Vector3(foot.x, foot.y + style.baseHeight * 0.5f, foot.z);
			plate.size = Vector3(style.baseWidth, style.baseHeight, style.baseWidth);
			plate.values.color = style.base.ToShaderValue();
			plate.values.darkColor = Vector4(0.0f, 0.0f, 0.0f, static_cast<float>(Shape::Solid));
			plate.values.center = Vector4(plate.center.x, plate.center.y, plate.center.z, style.baseHeight * 0.5f);
			plate.values.param = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			plate.values.litColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			return plate;
		}


		GimmickMarkRenderer::MarkPlate GimmickMarkRenderer::MakeFlagPole(const Vector3& foot) const
		{
			const FlagStyle& style = m_parameter.flag;

			MarkPlate plate;
			plate.center = Vector3(foot.x, foot.y + style.baseHeight + style.poleHeight * 0.5f, foot.z);
			plate.size = Vector3(style.poleWidth, style.poleHeight, style.poleWidth);
			plate.values.color = style.pole.ToShaderValue();
			plate.values.darkColor = Vector4(0.0f, 0.0f, 0.0f, static_cast<float>(Shape::Solid));
			plate.values.center = Vector4(plate.center.x, plate.center.y, plate.center.z, style.poleHeight * 0.5f);
			plate.values.param = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			plate.values.litColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			return plate;
		}


		GimmickMarkRenderer::MarkPlate GimmickMarkRenderer::MakeFlagKnob(const Vector3& foot) const
		{
			const FlagStyle& style = m_parameter.flag;
			// 玉はポールより手前に少し出す（ポールの太さの半分 + すき間）
			const float z = foot.z - style.poleWidth * 0.5f - m_parameter.placement.surfaceGap;

			MarkPlate plate;
			plate.center = Vector3(foot.x, foot.y + GetPoleTopHeight(), z);
			plate.size = Vector3(style.knobSize, style.knobSize, FLAG_CLOTH_THICKNESS);
			plate.values.color = style.knob.ToShaderValue();
			plate.values.darkColor = Vector4(style.outline.rgb[0], style.outline.rgb[1], style.outline.rgb[2], static_cast<float>(Shape::Ball));
			plate.values.center = Vector4(plate.center.x, plate.center.y, plate.center.z, style.knobSize * 0.5f);
			plate.values.param = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			plate.values.litColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			return plate;
		}


		GimmickMarkRenderer::MarkPlate GimmickMarkRenderer::MakeFlagCloth(const Vector3& foot, const FlagMotion& motion) const
		{
			const FlagStyle& style = m_parameter.flag;
			const float popScale = motion.GetPopScale(style);
			const float width = math::Max(style.clothWidth, 0.0f) * popScale;
			const float height = math::Max(style.clothHeight, 0.0f) * popScale;
			const FlagWave wave = motion.GetWave(style);
			// ポールの右側に付ける（上の端はポールのてっぺんより少し下。大きくなるときもポール側の上の端はそのまま）
			const float top = foot.y + GetPoleTopHeight() - FLAG_CLOTH_TOP_GAP;
			const float left = foot.x + style.poleWidth * 0.5f;
			// 旗はポールの手前の面にそろえる（ポールと同じ奥行きの位置だと、ポールの中に埋まる）
			const float z = foot.z - style.poleWidth * 0.5f;
			// はためいて上下にはみ出す分だけ、板を大きくしておく（形はシェーダーが作り、外側は捨てる）
			const float waveMargin = math::Max(wave.amplitude, 0.0f) * height * 0.5f;

			MarkPlate plate;
			plate.center = Vector3(left + width * 0.5f, top - height * 0.5f, z);
			plate.size = Vector3(width, height + waveMargin * 2.0f, FLAG_CLOTH_THICKNESS);
			// 白 → 赤（色も光る強さも混ぜる）
			plate.values.color = LerpMarkColor(style.clothOff, style.clothOn, motion.changeRate).ToShaderValue();
			plate.values.darkColor = Vector4(style.outline.rgb[0], style.outline.rgb[1], style.outline.rgb[2], static_cast<float>(Shape::Flag));
			plate.values.center = Vector4(plate.center.x, plate.center.y, plate.center.z, height * 0.5f);
			// x = 波の角度, y = 振れ幅, z = 幅の半分（高さの半分を 1 とした長さ）, w = 波の数
			const float halfWidthRate = height > 0.0f ? width / height : 0.0f;
			plate.values.param = Vector4(motion.phase, wave.amplitude, halfWidthRate, style.waveCount);
			plate.values.litColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			return plate;
		}


		Vector3 GimmickMarkRenderer::GetFlagFoot(const Checkpoint& checkpoint) const
		{
			const math::Vec2 spawn = checkpoint.GetSpawnPosition();
			return Vector3(spawn.x, spawn.y, m_parameter.flag.z);
		}


		float GimmickMarkRenderer::GetPoleTopHeight() const
		{
			const FlagStyle& style = m_parameter.flag;
			return style.baseHeight + style.poleHeight;
		}


		void GimmickMarkRenderer::FlagMotion::Advance(const FlagStyle& style, const bool isReached, const float deltaTime)
		{
			// 通ったら少しずつ赤へ。通っていない（ステージを読み直した）ならすぐ白に戻す
			if (isReached)
			{
				const float changeTime = math::Max(style.changeTime, 0.01f);
				changeRate = math::MoveTowards(changeRate, 1.0f, deltaTime / changeTime);
			}
			else
			{
				changeRate = 0.0f;
			}
			// はためき（赤くなるほど速い）
			phase += GetWave(style).speed * deltaTime;
			if (phase > FLAG_PHASE_WRAP)
			{
				phase -= FLAG_PHASE_WRAP;
			}
		}


		FlagWave GimmickMarkRenderer::FlagMotion::GetWave(const FlagStyle& style) const
		{
			FlagWave wave;
			wave.amplitude = math::Lerp(style.waveOff.amplitude, style.waveOn.amplitude, changeRate);
			wave.speed = math::Lerp(style.waveOff.speed, style.waveOn.speed, changeRate);
			return wave;
		}


		float GimmickMarkRenderer::FlagMotion::GetPopScale(const FlagStyle& style) const
		{
			// 変わり始めと変わり終わりは 1 倍、真ん中で一番大きい（山なり）
			return 1.0f + style.popScale * std::sin(math::PI * changeRate);
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
			// 鏡の奥行きの真ん中あたり（レールが鏡の真ん中を通っているように見せる）
			// ただし、光（Z = 0 を中心に、太さと同じ奥行き）より手前に出ると光を隠すので、光のすぐ奥に置く
			const MarkPlacement& placement = m_parameter.placement;
			return placement.lightBeamWidth * 0.5f + placement.surfaceGap + m_parameter.rail.depth * 0.5f;
		}
	}
}
