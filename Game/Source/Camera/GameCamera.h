#pragma once
#include "Source/Camera/CameraParameter.h"
#include "Source/Core/Math/Rect.h"


/**
 * @file GameCamera.h
 * @brief インゲームのカメラ（追いかける・全体表示・投影の設定）
 */
namespace app
{
	namespace camera
	{
		/**
		 * @brief インゲームのカメラ
		 * @details ・エンジンのカメラ（g_camera3D）を外から受け取って動かす（自分では作らない）
		 *          ・NewGO せず、シーンが直接持って Update を呼ぶ
		 *            （プレイヤーの更新のあと、カメラの位置を使う処理（鏡のガラス・ImGui の重ね描き）の前に呼ぶため）
		 *          ・追いかける相手（プレイヤー）は知らない。毎フレーム「見たい位置」を渡してもらう
		 *            （プレイヤー以外を映したいとき・演出で別の場所を映すときも、このクラスは変えなくてよい）
		 *          ・ステージも知らない。全体表示に使う「ステージの範囲」は SetStageBounds で渡してもらう
		 *          ・2つの見え方を切り替えられる（SetOverview / ToggleOverview）
		 *            ふだん   … 見たい位置（プレイヤー）をなめらかに追いかける
		 *            全体表示 … ステージの範囲の真ん中を、範囲全体が画面に入る広さで映す
		 *            切り替えたときは、注視点とズームが smoothing の速さで動くので、すっと引いたり寄ったりする
		 *          ・調整用の ImGui は CameraEditor にある（このクラスは ImGui を使わない）
		 */
		class GameCamera
		{
		public:
			GameCamera();
			~GameCamera() = default;

			/**
			 * @brief 初期化（投影の設定をカメラに反映する）
			 * @param camera 動かすエンジンのカメラ（ふつうは g_camera3D）
			 * @param parameter 調整値（ResetParameter で戻る値にもなる）
			 */
			void Init(Camera* camera, const CameraParameter& parameter = CameraParameter());
			/**
			 * @brief 毎フレームの更新（見たい位置へなめらかに追いかける。全体表示のときはステージの真ん中へ）
			 * @param deltaTime 経過時間（秒）
			 * @param followPosition 見たい位置（プレイヤーの足元など。z は奥行き）
			 */
			void Update(float deltaTime, const Vector3& followPosition);
			/**
			 * @brief 補間せずにすぐ追いつく（開始・リスタート・ワープの直後）
			 * @param followPosition 見たい位置
			 */
			void SnapTo(const Vector3& followPosition);

			// ---- 全体表示 ----
			/**
			 * @brief ステージの範囲を設定する（全体表示で映す範囲。ステージを読み直したら設定し直す）
			 * @param bounds ステージの範囲（左下 min・右上 max）
			 */
			void SetStageBounds(const math::Rect& bounds);
			/** ステージの範囲を消す（全体表示にしても、ふだんの追いかける見え方のままになる） */
			void ClearStageBounds() { m_hasStageBounds = false; }
			/** ステージの範囲が設定されているか */
			bool HasStageBounds() const { return m_hasStageBounds; }
			/** ステージの範囲 */
			const math::Rect& GetStageBounds() const { return m_stageBounds; }
			/** 全体表示にする・やめる */
			void SetOverview(const bool isOverview) { m_parameter.isOverview = isOverview; }
			/** 全体表示とふだんの見え方を切り替える */
			void ToggleOverview() { m_parameter.isOverview = !m_parameter.isOverview; }
			/** 全体表示か */
			bool IsOverview() const { return m_parameter.isOverview; }

			// ---- 調整値 ----
			/** 調整値 */
			const CameraParameter& GetParameter() const { return m_parameter; }
			/** 調整値（書き換え用） */
			CameraParameter& GetParameterForEdit() { return m_parameter; }
			/** 調整値を Init で渡された値に戻す */
			void ResetParameter() { m_parameter = m_initialParameter; }

			// ---- 今の状態（確認用） ----
			/** 今の注視点（補間中の値） */
			const Vector3& GetCurrentTarget() const { return m_currentTarget; }
			/** 今のズーム倍率（補間中の値。平行投影で使う） */
			float GetCurrentZoom() const { return m_currentZoom; }
			/** 今のカメラの距離（補間中の値） */
			float GetCurrentDistance() const { return m_currentDistance; }
			/** 今、画面の縦に映っている長さ（平行投影のとき） */
			float GetCurrentViewHeight() const { return m_parameter.baseViewHeight * m_currentZoom; }
			/** 画面の横 ÷ 縦 */
			float GetAspect() const;

		private:
			/**
			 * @brief 見たい位置へ近づける
			 * @param followPosition 見たい位置
			 * @param rate 近づける割合（0 で動かない、1 でぴったり追いつく）
			 */
			void Follow(const Vector3& followPosition, float rate);
			/**
			 * @brief 目指す注視点・ズーム・距離を決める
			 * @param followPosition 見たい位置
			 * @param outTarget 目指す注視点
			 * @param outZoom 目指すズーム（平行投影）
			 * @param outDistance 目指す距離
			 */
			void CalcDesiredView(const Vector3& followPosition, Vector3& outTarget, float& outZoom, float& outDistance) const;
			/** 投影の設定（平行投影の幅・高さ、画角、近平面・遠平面）をカメラに反映する */
			void ApplyProjection();
			/** 注視点とカメラの位置をカメラに反映する */
			void ApplyView();

		private:
			/** 動かすエンジンのカメラ（持ち主ではない） */
			Camera* m_camera;
			/** 調整値 */
			CameraParameter m_parameter;
			/** Init で渡された調整値（ResetParameter 用） */
			CameraParameter m_initialParameter;
			/** 今の注視点（補間中） */
			Vector3 m_currentTarget;
			/** 今のカメラの距離（補間中） */
			float m_currentDistance;
			/** 今のズーム倍率（補間中） */
			float m_currentZoom;
			/** ステージの範囲（全体表示で映す範囲） */
			math::Rect m_stageBounds;
			/** ステージの範囲が設定されているか */
			bool m_hasStageBounds;
		};
	}
}
