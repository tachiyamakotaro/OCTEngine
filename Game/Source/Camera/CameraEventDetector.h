#pragma once
#include <vector>


/**
 * @file CameraEventDetector.h
 * @brief ステージのギミックの変化から、カメラが寄る場面を見つける
 */
namespace app
{
	namespace stage
	{
		class Stage;
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
		 *            どちらも、ステージエディタで「Camera Focus」にしたギミックだけ（毎回寄ると酔いやすいので、寄る所を選ぶ）
		 *            ゴールはシーンが決める出来事なので、ゴールに触れた所（DebugScene）から EventCamera::Play を呼ぶ
		 *          ・GameplaySEPlayer と同じく「前のフレームから何が変わったか」を見る。
		 *            ギミックのクラスにはカメラのことを書かない（Stage・ギミックはカメラを知らないまま）
		 *          ・同じ扉で何度も寄らないように、一度開いた扉は覚えておく（リセットするまで寄らない）
		 *          ・寄り方（どれだけ寄るか・何秒か）は決めない。見つけた場面を EventCamera に渡すだけ
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

		private:
			/** 扉が初めて開き始めたか（見つけたら寄る） */
			void DetectDoorOpened(const stage::Stage& stage, EventCamera& eventCamera);
			/** 溶かす氷に通れる穴が開いたか（見つけたら寄る） */
			void DetectIcePassable(const stage::Stage& stage, EventCamera& eventCamera) const;
			/** 覚えている扉の数が今のステージと同じか（エディタで足したり消したりしたら違う） */
			bool IsSameLayout(const stage::Stage& stage) const;

		private:
			/** 扉ごとの、もう開いたことがあるか（並びはステージの扉の並びと同じ） */
			std::vector<bool> m_hasDoorOpened;
		};
	}
}
