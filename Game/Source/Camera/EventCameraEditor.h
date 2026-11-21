#pragma once
#include "Source/Camera/CameraEvent.h"


/**
 * @file EventCameraEditor.h
 * @brief イベントカメラを調整する ImGui（デバッグウィンドウの「Event Camera」の中身）
 */
namespace app
{
	namespace camera
	{
		class EventCamera;
		struct EventShotSetting;


		/**
		 * @brief イベントカメラを調整する ImGui
		 * @details ・EventCamera は ImGui を使わない。ImGui の項目はこのクラスにまとめる
		 *          ・ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 *          ・場面ごとに「Test」ボタンがあり、ギミックを解かなくても寄り方を試せる（プレイヤーだけを映す）
		 */
		class EventCameraEditor
		{
		public:
			EventCameraEditor() = default;
			~EventCameraEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param eventCamera 調整するイベントカメラ
			 */
			void Draw(EventCamera& eventCamera);

		private:
			/** 今の状態（演出中の場面・効き具合）と、止めるボタン */
			void DrawStatus(EventCamera& eventCamera);
			/**
			 * @brief 場面1つぶんの寄り方
			 * @param eventCamera 試し再生する相手
			 * @param kind 場面の種類
			 * @param setting その場面の寄り方
			 */
			void DrawShotSetting(EventCamera& eventCamera, CameraEventKind kind, EventShotSetting& setting);
		};
	}
}
