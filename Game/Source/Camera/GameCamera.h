#pragma once
#include "Source/Camera/CameraParameter.h"


/**
 * @file GameCamera.h
 * @brief インゲームのカメラ（追いかける・投影の設定・調整用の ImGui）
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
		 */
		class GameCamera
		{
		public:
			GameCamera();
			~GameCamera() = default;

			/**
			 * @brief 初期化（投影の設定をカメラに反映する）
			 * @param camera 動かすエンジンのカメラ（ふつうは g_camera3D）
			 * @param parameter 調整値（ImGui の Reset で戻る値にもなる）
			 */
			void Init(Camera* camera, const CameraParameter& parameter = CameraParameter());
			/**
			 * @brief 毎フレームの更新（見たい位置へなめらかに追いかける）
			 * @param deltaTime 経過時間（秒）
			 * @param followPosition 見たい位置（プレイヤーの足元など。z は奥行き）
			 */
			void Update(float deltaTime, const Vector3& followPosition);
			/**
			 * @brief 補間せずにすぐ追いつく（開始・リスタート・ワープの直後）
			 * @param followPosition 見たい位置
			 */
			void SnapTo(const Vector3& followPosition);
			/** ImGui の調整項目を描く（ウィンドウの中で呼ぶ。Begin / End はしない） */
			void DrawSettingsUI();

			/** 調整値 */
			const CameraParameter& GetParameter() const { return m_parameter; }
			/** 調整値（書き換え用） */
			CameraParameter& GetParameterForEdit() { return m_parameter; }
			/** 今の注視点（補間中の値） */
			const Vector3& GetCurrentTarget() const { return m_currentTarget; }

		private:
			/**
			 * @brief 見たい位置へ近づける
			 * @param followPosition 見たい位置
			 * @param rate 近づける割合（0 で動かない、1 でぴったり追いつく）
			 */
			void Follow(const Vector3& followPosition, float rate);
			/** 投影の設定（平行投影の幅・高さ、画角、近平面・遠平面）をカメラに反映する */
			void ApplyProjection();
			/** 注視点とカメラの位置をカメラに反映する */
			void ApplyView();

		private:
			/** 動かすエンジンのカメラ（持ち主ではない） */
			Camera* m_camera;
			/** 調整値 */
			CameraParameter m_parameter;
			/** Init で渡された調整値（ImGui の Reset 用） */
			CameraParameter m_initialParameter;
			/** 今の注視点（補間中） */
			Vector3 m_currentTarget;
			/** 今のカメラの距離（補間中） */
			float m_currentDistance;
		};
	}
}
