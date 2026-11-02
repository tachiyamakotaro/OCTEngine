#pragma once


/**
 * @file CameraEditor.h
 * @brief ゲームカメラを調整する ImGui（デバッグウィンドウの「Camera」の中身）
 */
namespace app
{
	namespace camera
	{
		class GameCamera;


		/**
		 * @brief ゲームカメラを調整する ImGui
		 * @details ・GameCamera は ImGui を使わず、カメラの動きだけを持つ。ImGui の項目はこのクラスにまとめる
		 *            （シーンに書くのは Draw を1行呼ぶだけでよい）
		 *          ・ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 *          ・見え方の切り替え：Follow（ふだんの平行投影でプレイヤーを追う）／ Whole Stage（ステージ全体）
		 */
		class CameraEditor
		{
		public:
			CameraEditor() = default;
			~CameraEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param camera 調整するカメラ
			 */
			void Draw(GameCamera& camera);

		private:
			/** 見え方の切り替え（ふだん／全体表示）と、全体表示の調整 */
			void DrawViewMode(GameCamera& camera);
			/** 投影（平行投影・ズーム・画角） */
			void DrawProjection(GameCamera& camera);
			/** 追従（距離・高さ・なめらかさ） */
			void DrawFollow(GameCamera& camera);
			/** 描画の範囲（近平面・遠平面）と、今の状態の表示・リセット */
			void DrawClipAndStatus(GameCamera& camera);
		};
	}
}
