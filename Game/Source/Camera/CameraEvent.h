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
			StageStart,		//!< ステージが始まった（寄った所から、丸が開くのに合わせて引いていく）
			Death,			//!< プレイヤーが死んだ（リスタートまで寄ったまま）
		};


		/** 種類の名前（デバッグ表示用） */
		inline const char* CameraEventKindToString(const CameraEventKind kind)
		{
			switch (kind)
			{
			case CameraEventKind::GimmickSolved: return "Gimmick Solved";
			case CameraEventKind::Goal:          return "Goal";
			case CameraEventKind::StageStart:    return "Stage Start";
			case CameraEventKind::Death:         return "Death";
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
			static CameraEvent GimmickSolved(const math::Rect& area) { return CameraEvent(CameraEventKind::GimmickSolved, area, true, false); }
			/** ゴールに触れた（area = ゴールの範囲。プレイヤーと一緒に映す） */
			static CameraEvent Goal(const math::Rect& area) { return CameraEvent(CameraEventKind::Goal, area, true, false); }
			/**
			 * @brief ステージが始まった（プレイヤーだけを映す）
			 * @details EventCamera::Release を呼ぶまで寄ったままにする（丸が開き始めたら、シーンが Release を呼ぶ）
			 */
			static CameraEvent StageStart() { return CameraEvent(CameraEventKind::StageStart, math::Rect(), false, true); }
			/** プレイヤーが死んだ（プレイヤーだけを映す。リスタートで Reset されるまで寄ったまま） */
			static CameraEvent Death() { return CameraEvent(CameraEventKind::Death, math::Rect(), false, true); }
			/** プレイヤーだけを映す（ImGui の試し再生など。Hold の時間がたったら戻る） */
			static CameraEvent PlayerOnly(const CameraEventKind kind) { return CameraEvent(kind, math::Rect(), false, false); }

			CameraEvent() : CameraEvent(CameraEventKind::GimmickSolved, math::Rect(), false, false) {}

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
			/** 優先度（大きいほど大事。演出中に来た場面は、今と同じか今より大事なときだけ入れ替える） */
			int GetPriority() const
			{
				switch (m_kind)
				{
				case CameraEventKind::Goal:       return 3;
				case CameraEventKind::Death:      return 2;
				case CameraEventKind::StageStart: return 1;
				default:                          return 0;
				}
			}
			/** EventCamera::Release を呼ぶまで寄ったままにするか（false なら Hold の時間で戻る） */
			bool IsHeldUntilRelease() const { return m_isHeldUntilRelease; }

		private:
			CameraEvent(const CameraEventKind kind, const math::Rect& targetArea, const bool hasTarget, const bool isHeldUntilRelease)
				: m_kind(kind), m_targetArea(targetArea), m_hasTarget(hasTarget), m_isHeldUntilRelease(isHeldUntilRelease) {}

		private:
			/** 場面の種類 */
			CameraEventKind m_kind;
			/** 一緒に映したい範囲（解けたギミック・ゴール） */
			math::Rect m_targetArea;
			/** 一緒に映したい範囲があるか */
			bool m_hasTarget;
			/** Release を呼ぶまで寄ったままにするか */
			bool m_isHeldUntilRelease;
		};
	}
}
