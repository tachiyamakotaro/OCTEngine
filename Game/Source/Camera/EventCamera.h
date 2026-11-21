#pragma once
#include "Source/Camera/CameraEvent.h"
#include "Source/Camera/CameraShot.h"
#include "Source/Camera/EventCameraParameter.h"


/**
 * @file EventCamera.h
 * @brief イベントカメラ（ギミックを解いた・ゴールなど、特定の場面でだけカメラを寄せる）
 */
namespace app
{
	namespace camera
	{
		class GameCamera;


		/**
		 * @brief イベントカメラ
		 * @details ・ふだんは何もしない。Play で場面を渡されたら、寄る → 寄ったまま → ふだんに戻る を時間で進める
		 *          ・「どの場面か」を見つけるのは CameraEventDetector（とゴールに触れたシーン）。このクラスはステージを知らない
		 *          ・カメラを直接動かさない。毎フレーム CameraShot（映す範囲・寄り具合・効き具合）を作り、ApplyTo で GameCamera に渡す
		 *            （GameCamera はふだんの見え方とこれを混ぜるだけ。全体表示のときは使わない）
		 *          ・映す範囲は毎フレーム、プレイヤーの今の体の範囲から作り直す（寄っている間にプレイヤーが動いても画面から出ない）
		 *          ・演出中に別の場面が来たら、今より大事な場面（GetPriority が大きい）か同じくらいの場面のときだけ入れ替える
		 *          ・調整用の ImGui は EventCameraEditor にある（このクラスは ImGui を使わない）
		 */
		class EventCamera
		{
		public:
			EventCamera();
			~EventCamera() = default;

			/**
			 * @brief 初期化
			 * @param parameter 調整値
			 */
			void Init(const EventCameraParameter& parameter = EventCameraParameter());
			/**
			 * @brief 場面に合わせて寄り始める
			 * @details その場面で寄らない設定（isEnabled = false）のときと、今の演出のほうが大事なときは何もしない
			 */
			void Play(const CameraEvent& event);
			/** 演出をやめて、すぐにふだんの見え方に戻す（リスタート・ステージの読み直し） */
			void Reset();
			/**
			 * @brief 時間を進めて、映してほしい見え方を作る（毎フレーム。カメラを動かす前に呼ぶ）
			 * @param deltaTime 経過時間（秒）
			 * @param playerBody プレイヤーの体の範囲（映す範囲の真ん中になる）
			 */
			void Update(float deltaTime, const math::Rect& playerBody);
			/** 作った見え方をカメラに渡す（演出していないときは「効き具合 0」を渡すので、ふだんの見え方になる） */
			void ApplyTo(GameCamera& camera) const;

			// ---- 調整値 ----
			const EventCameraParameter& GetParameter() const { return m_parameter; }
			EventCameraParameter& GetParameterForEdit() { return m_parameter; }

			// ---- 今の状態（確認用） ----
			/** 演出中か */
			bool IsPlaying() const { return m_isPlaying; }
			/** 演出中の場面 */
			const CameraEvent& GetPlayingEvent() const { return m_event; }
			/** 今の効き具合（0〜1） */
			float GetWeight() const { return m_shot.weight; }

		private:
			/** 場面の種類ごとの寄り方 */
			const EventShotSetting& GetSetting(CameraEventKind kind) const;
			/** 寄る時間・寄ったままの時間・戻る時間を全部足した長さ */
			float CalcTotalTime(const EventShotSetting& setting) const;
			/** 経過時間から効き具合（0〜1）を求める */
			float CalcWeight(const EventShotSetting& setting) const;

		private:
			/** 調整値 */
			EventCameraParameter m_parameter;
			/** 演出中の場面 */
			CameraEvent m_event;
			/** 演出を始めてからの時間 */
			float m_elapsed;
			/** 演出中か */
			bool m_isPlaying;
			/** 今、映してほしい見え方 */
			CameraShot m_shot;
		};
	}
}
