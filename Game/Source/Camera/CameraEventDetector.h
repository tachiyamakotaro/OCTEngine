#pragma once
#include <vector>
#include "Source/Core/Math/Rect.h"


/**
 * @file CameraEventDetector.h
 * @brief ステージのギミックの変化から、カメラが寄る場面を見つける
 */
namespace app
{
	namespace stage
	{
		class Stage;
		class GimmickBase;
		class LightMirror;
	}


	namespace camera
	{
		class EventCamera;


		/**
		 * @brief ステージのギミックの変化から、カメラが寄る場面を見つける
		 * @details ・「どういう場面で寄るか」はこのクラスにまとめる（場面ごとに Detect○○ の関数を1つ）
		 *            今の場面
		 *            ・扉が初めて開き始めた（受光器・スイッチ・色センサーなど、扉につながったギミックを解いた）
		 *            ・溶かす氷（IceKind::Melt）に、プレイヤーが通れる穴が開いた
		 *            ・ソーラーパネルの動く床・壁（MovingBlock）が初めて動き出した
		 *            ・反射鏡（LightMirror）を正しい向きにして、初めて光を反射した
		 *            どれも、ステージエディタで「Camera Focus」にしたギミックだけ（毎回寄ると酔いやすいので、寄る所を選ぶ）
		 *            ゴールはシーンが決める出来事なので、ゴールに触れた所（DebugScene）から EventCamera::Play を呼ぶ
		 *          ・GameplaySEPlayer と同じく「前のフレームから何が変わったか」を見る。
		 *            ギミックのクラスにはカメラのことを書かない（Stage・ギミックはカメラを知らないまま）
		 *          ・同じ扉・動く床・鏡で何度も寄らないように、一度開いた扉・一度動いた床・一度そろえた鏡は覚えておく（リセットするまで寄らない）
		 *          ・寄り方（どれだけ寄るか・何秒か）は決めない。見つけた場面を EventCamera に渡すだけ
		 *          ・最後に寄ったギミックを覚えておく（寄っている間、そのギミックだけ動かすのに使う）
		 *
		 *          使い方（DebugScene）
		 *          ・Start とリスタートのあとに Reset（今の状態を覚えるだけで、寄らない）
		 *          ・毎フレーム、ステージ → プレイヤー → 鏡の操作 の更新が終わったあとに Update
		 */
		class CameraEventDetector
		{
		public:
			CameraEventDetector();
			~CameraEventDetector() = default;

			/**
			 * @brief 今の状態を覚え直す（寄らない）
			 * @details 呼ばないと、リスタートで開いたままになっている扉などで寄ってしまう
			 */
			void Reset(const stage::Stage& stage);
			/**
			 * @brief 前のフレームからの変化を見て、寄る場面があれば eventCamera に渡す（毎フレーム）
			 * @param stage ステージ（Update が終わったもの）
			 * @param eventCamera 寄る場面を渡す相手
			 */
			void Update(const stage::Stage& stage, EventCamera& eventCamera);
			/**
			 * @brief 最後に寄ったギミック（まだ寄っていない・リセットしたあとは nullptr）
			 * @details EventCamera::IsLockingPlayer の間（寄っている間）だけ使う。
			 *          エディタで消されたギミックを指していることもあるので、中身は見ずに「同じものか」を比べるだけにする
			 */
			const stage::GimmickBase* GetFocusedGimmick() const { return m_focusedGimmick; }

		private:
			/** 扉が初めて開き始めたか（見つけたら寄る） */
			void DetectDoorOpened(const stage::Stage& stage, EventCamera& eventCamera);
			/** 溶かす氷に通れる穴が開いたか（見つけたら寄る） */
			void DetectIcePassable(const stage::Stage& stage, EventCamera& eventCamera);
			/** 動く床・壁が初めて動き出したか（見つけたら寄る） */
			void DetectBlockStarted(const stage::Stage& stage, EventCamera& eventCamera);
			/** 鏡が正しい向きで初めて光を反射したか（見つけたら寄る） */
			void DetectMirrorAligned(const stage::Stage& stage, EventCamera& eventCamera);
			/**
			 * @brief 鏡をそろえたときに映す範囲
			 * @details 暗がり（ステージ全体ではない物）の中の鏡なら、暗がりの真ん中あたり。暗がりの外なら、鏡と反射した光の行き先
			 */
			math::Rect GetMirrorFocusArea(const stage::Stage& stage, const stage::LightMirror& mirror) const;
			/** 覚えている扉・動く床・鏡の数が今のステージと同じか（エディタで足したり消したりしたら違う） */
			bool IsSameLayout(const stage::Stage& stage) const;
			/** gimmick に寄る（EventCamera に渡して、寄ったギミックとして覚える） */
			void Focus(const stage::GimmickBase& gimmick, const math::Rect& area, EventCamera& eventCamera);

		private:
			/** 扉ごとの、もう開いたことがあるか（並びはステージの扉の並びと同じ） */
			std::vector<bool> m_hasDoorOpened;
			/** 動く床・壁ごとの、もう動いたことがあるか（並びはステージの動く床の並びと同じ） */
			std::vector<bool> m_hasBlockStarted;
			/** 鏡ごとの、もう正しい向きで光を反射したことがあるか（並びはステージの鏡の並びと同じ） */
			std::vector<bool> m_hasMirrorAligned;
			/** 最後に寄ったギミック（中身は見ない。同じものか比べるだけ） */
			const stage::GimmickBase* m_focusedGimmick;
		};
	}
}
