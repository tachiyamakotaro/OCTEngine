#pragma once
#include "Source/Stage/GimmickMarkParameter.h"
#include "Source/Stage/Gimmick/LightMirror.h"
#include <memory>
#include <type_traits>
#include <vector>


/**
 * @file GimmickMarkRenderer.h
 * @brief ギミックに「何ができるか」の印を描く（受光器の電球マーク・回せる反射鏡の歯車・動かせる反射鏡のレール）
 */
namespace app
{
	namespace stage
	{
		class Stage;
		class LightReceiver;


		/**
		 * @brief ギミックの印を描く
		 * @details ・遠くから見ても、そのギミックで何ができるかわかるようにするための印
		 *            受光器（黄色）… 手前の面に電球マーク。光が当たっていなくても白っぽく少し光る。当たると強く光り、光の線が出る
		 *            回せる反射鏡   … 回る軸の手前に歯車。鏡と一緒に回る
		 *            動かせる反射鏡 … 動かせる範囲にレールと、両端の止め金。範囲を伸ばすとレールも伸びる
		 *          ・どこに・どの長さで描くかは、ギミックに聞かずに答えてもらう（LightMirror::HasMoveRange / GetMoveRange）
		 *          ・印は白いテクスチャの箱（solar_panel.tkm）を薄い板にして置き、形と色はシェーダー（Assets/shader/gimmickMark.fx）が作る
		 *          ・印ごとに色や光り具合が違うので、1つずつ自分の定数バッファを持つ（StageRenderer のソーラーパネルと同じ作り）
		 *          ・奥の鏡に映った像は描かない（今は全ステージで奥の鏡を使っていないため。FireGimmickRenderer と同じ）
		 *          ・調整用の ImGui は GimmickMarkEditor にある（このクラスは ImGui を使わない）
		 */
		class GimmickMarkRenderer
		{
		public:
			/**
			 * @brief 印の形（シェーダーの SHAPE_～ と同じ番号）
			 */
			enum class Shape : int
			{
				Solid = 0,	//!< ただの箱（レールの止め金）
				Bulb = 1,	//!< 電球
				Gear = 2,	//!< 歯車
				Rail = 3,	//!< レール
			};


		public:
			GimmickMarkRenderer() = default;
			~GimmickMarkRenderer() = default;
			// Model を持っている（Model は定数バッファの場所を覚えている）のでコピー禁止
			GimmickMarkRenderer(const GimmickMarkRenderer&) = delete;
			GimmickMarkRenderer& operator=(const GimmickMarkRenderer&) = delete;

			/**
			 * @brief 初期化（モデルは必要になったときに読み込む）
			 * @param parameter 調整値（ResetParameter で戻る値にもなる）
			 */
			void Init(const GimmickMarkParameter& parameter = GimmickMarkParameter());
			/**
			 * @brief ステージに合わせて印を置く（毎フレーム。鏡の操作のあとに呼ぶ）
			 */
			void Update(const Stage& stage);
			/**
			 * @brief 描く（不透明。GimmickRenderer の Draw のあとに呼ぶ）
			 */
			void Draw(RenderContext& rc);

			// ---- 調整値 ----
			/** 調整値 */
			const GimmickMarkParameter& GetParameter() const { return m_parameter; }
			/** 調整値（書き換え用） */
			GimmickMarkParameter& GetParameterForEdit() { return m_parameter; }
			/** 調整値を Init で渡された値に戻す */
			void ResetParameter() { m_parameter = m_initialParameter; }

			// ---- 今の状態（確認用） ----
			/** 今フレーム置いた印の数 */
			size_t GetCount() const { return m_count; }

		private:
			/** SceneLight のライトの構造体（private な型なので、GetSceneLight の戻り値から型を取る） */
			using SceneLightData = std::remove_reference_t<decltype(SceneLight::GetInstance()->GetSceneLight())>;

			/**
			 * @brief シェーダーに渡す印の値（gimmickMark.fx の MODEL_CB_EXTRA と同じ並び）
			 */
			struct MarkValues
			{
				/** rgb = 地の色, a = 自分で光る強さ */
				Vector4 color;
				/** rgb = 縁取り・暗い所の色, a = 形（Shape） */
				Vector4 darkColor;
				/** xyz = 印の中心（ワールド座標）, w = 印の大きさの半分（レールは高さの半分） */
				Vector4 center;
				/** 形ごとの値（電球：y = 光り具合 ／ 歯車：x = 回転, w = 歯の数 ／ レール：z = 長さの半分, w = 枕木の間隔） */
				Vector4 param;
				/** rgb = 光っているときの色, a = 光っているときに自分で光る強さ（電球だけ使う） */
				Vector4 litColor;
			};
			/**
			 * @brief 置く板1枚ぶん（どこに・どの大きさで・どう描くか）
			 */
			struct MarkPlate
			{
				/** 板の中心（ワールド座標） */
				Vector3 center;
				/** 板の大きさ（X:幅 / Y:高さ / Z:奥行き） */
				Vector3 size;
				/** シェーダーに渡す値 */
				MarkValues values;
			};
			/**
			 * @brief シェーダーに渡す値（前半は model.fx と同じライトの値、後ろに印の値をつなげる）
			 */
			struct Constants
			{
				SceneLightData light;
				MarkValues mark;
			};
			static_assert(sizeof(SceneLightData) % 16 == 0, "SceneLight の構造体の大きさが 16 の倍数でないと、HLSL とずれる");
			static_assert(sizeof(MarkValues) == 16 * 5, "MarkValues は float4 が5つ（gimmickMark.fx と同じ並び）");
			/**
			 * @brief 印1つぶんのモデルと、そのモデル専用のシェーダーに渡す値
			 */
			struct MarkModel
			{
				Model model;
				Constants constants;
			};

		private:
			/** 受光器の電球マークを置く */
			void PutBulbs(const Stage& stage);
			/** 反射鏡の歯車とレールを置く */
			void PutMirrorMarks(const Stage& stage);

			/** 受光器の電球マークの板 */
			MarkPlate MakeBulb(const LightReceiver& receiver) const;
			/** 回せる反射鏡の歯車の板（鏡の角度だけ回す） */
			MarkPlate MakeGear(const LightMirror& mirror) const;
			/** 動かせる反射鏡のレールの板 */
			MarkPlate MakeRail(const LightMirror::MoveRange& range) const;
			/**
			 * @brief レールの端の止め金の板
			 * @param side -1 で左端、+1 で右端
			 */
			MarkPlate MakeStopper(const LightMirror::MoveRange& range, float side) const;

			/** 板を1枚置く */
			void Put(const MarkPlate& plate);
			/** 印のモデルを1つ取り出す（足りなければ作る） */
			MarkModel& Acquire();

			/** 歯車を置く奥行きの位置（鏡の手前の面のすぐ手前） */
			float GetGearZ() const;
			/** レールを置く奥行きの位置（鏡の奥の面のすぐ奥） */
			float GetRailZ() const;

		private:
			/** 調整値 */
			GimmickMarkParameter m_parameter;
			/** Init で渡された調整値（ResetParameter 用） */
			GimmickMarkParameter m_initialParameter;
			/** 印のモデル（unique_ptr なので、増えても constants の場所は変わらない。Model はこの場所を覚えている） */
			std::vector<std::unique_ptr<MarkModel>> m_marks;
			/** 今フレーム使っている数 */
			size_t m_count = 0;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
