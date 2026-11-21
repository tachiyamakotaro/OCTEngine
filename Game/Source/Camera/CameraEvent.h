#pragma once
#include "Source/Core/Math/Rect.h"


/**
 * @file CameraEvent.h
 * @brief カメラが寄る場面（出来事）
 */
namespace app
{
	namespace camera
	{
		/**
		 * @brief カメラが寄る場面の種類
		 * @details 場面を足すときは
		 *          ① ここに種類を足す
		 *          ② EventCameraParameter に調整値を足す（EventCamera::GetSetting と EventCameraEditor にも1行ずつ）
		 *          ③ 場面を見つける所で EventCamera::Play を呼ぶ
		 *             ・ステージのギミックの変化で決まる場面 … CameraEventDetector に関数を1つ足す
		 *             ・シーンの流れで決まる場面（ゴールなど）… その出来事が起きた所（DebugScene）から Play する
		 */
		enum class CameraEventKind
		{
			GimmickSolved,	//!< ギミックを解いた（扉が初めて開いた・溶かす氷に通れる穴が開いた）
			Goal,			//!< ゴールに触れた（クリアの演出の間）
		};


		/** 種類の名前（デバッグ表示用） */
		inline const char* CameraEventKindToString(const CameraEventKind kind)
		{
			switch (kind)
			{
			case CameraEventKind::GimmickSolved: return "Gimmick Solved";
			case CameraEventKind::Goal:          return "Goal";
			}
			return "Unknown";
		}


		/**
		 * @brief カメラが寄る場面（どの場面で、どこを一緒に映したいか）
		 * @details 作るときは下の関数を使う（例：CameraEvent::GimmickSolved(door.rect)）
		 */
		class CameraEvent
		{
		public:
			/** ギミックを解いた（area = 解けたギミックの範囲。プレイヤーと一緒に映す） */
			static CameraEvent GimmickSolved(const math::Rect& area) { return CameraEvent(CameraEventKind::GimmickSolved, area, true); }
			/** ゴールに触れた（area = ゴールの範囲。プレイヤーと一緒に映す） */
			static CameraEvent Goal(const math::Rect& area) { return CameraEvent(CameraEventKind::Goal, area, true); }
			/** プレイヤーだけを映す（ImGui の試し再生など） */
			static CameraEvent PlayerOnly(const CameraEventKind kind) { return CameraEvent(kind, math::Rect(), false); }

			CameraEvent() : CameraEvent(CameraEventKind::GimmickSolved, math::Rect(), false) {}

			/** 場面の種類 */
			CameraEventKind GetKind() const { return m_kind; }
			/**
			 * @brief プレイヤーと一緒に映す範囲を、body に足した範囲
			 * @param body プレイヤーの体の範囲
			 * @param isIncludeTarget ギミック・ゴールも一緒に映すか（false ならプレイヤーだけ）
			 */
			math::Rect MakeFocusArea(const math::Rect& body, const bool isIncludeTarget) const
			{
				return (m_hasTarget && isIncludeTarget) ? body.Merged(m_targetArea) : body;
			}
			/** 優先度（大きいほど大事。演出中に来た場面は、今より大事なときだけ入れ替える） */
			int GetPriority() const { return m_kind == CameraEventKind::Goal ? 1 : 0; }

		private:
			CameraEvent(const CameraEventKind kind, const math::Rect& targetArea, const bool hasTarget)
				: m_kind(kind), m_targetArea(targetArea), m_hasTarget(hasTarget) {}

		private:
			/** 場面の種類 */
			CameraEventKind m_kind;
			/** 一緒に映したい範囲（解けたギミック・ゴール） */
			math::Rect m_targetArea;
			/** 一緒に映したい範囲があるか */
			bool m_hasTarget;
		};
	}
}
