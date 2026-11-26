#pragma once
#include "Source/Title/TitleParameter.h"
#include <memory>


/**
 * @file TitlePlayerModel.h
 * @brief タイトル画面のプレイヤー（見た目だけ。遺跡の門の少し手前まで走ってきて待ち、遊ぶステージを決めたら門の前へ行って門のほうを向き、扉が開いたら門の中へ入っていく）
 */
namespace app
{
	namespace title
	{
		class EntranceTimeline;
		class RuinEntrance;


		/**
		 * @brief タイトル画面のプレイヤー（見た目だけ）
		 * @details ・ゲームのプレイヤーと同じモデルとアニメーション（Idle / Run）を使う。当たり判定や操作はない
		 *          ・位置とアニメーションは EntranceTimeline の進み具合から決める
		 *              走っている間 … Run で進む（始まったとき：左の外 → 門の少し手前、決めたとき：門の少し手前 → 門の前、
		 *                              扉が開いたら：門の前 → 門の中（扉の奥の光の板を通り抜けた奥））
		 *              それ以外     … Idle
		 *          ・門の前に着いたら、門のほう（奥）へ向きを変える（背中が見えて「これから入っていく」ように見せる）
		 *          ・扉が開いたら、まっすぐ奥を向いて門の中へ走って入る。門の足元の段は RuinEntrance に床の高さを聞いて上る
		 *            扉の奥の光の板を通り抜けた所から光の板に隠れるので、光の中へ吸い込まれて消えるように見える
		 *            入りきったら（GetWalkInRate が 1）描かない（影も落とさない）
		 *          ・モデルの大きさ（4.75）と向き（右向き 90 度）は PlayerParameter / CharacterBase と同じ
		 */
		class TitlePlayerModel
		{
		public:
			TitlePlayerModel() = default;
			~TitlePlayerModel() = default;
			TitlePlayerModel(const TitlePlayerModel&) = delete;
			TitlePlayerModel& operator=(const TitlePlayerModel&) = delete;

			void Init(const RuinEntranceLayout& layout);
			/**
			 * @brief 演出の進み具合に合わせて動かす（毎フレーム）
			 * @param entrance 門の前と門の中の床の高さを聞く（段を上るため）
			 */
			void Update(const EntranceTimeline& timeline, const RuinEntrance& entrance);
			void Draw(RenderContext& rc);

		private:
			/** アニメーションの番号（ANIMATION_FILES の並び） */
			enum class Animation
			{
				Idle,
				Run,
				Num,
			};

			/** アニメーションを変える（同じなら何もしない） */
			void Play(Animation animation);

		private:
			/** 置き方 */
			RuinEntranceLayout m_layout;
			/** モデル */
			ModelRender m_modelRender;
			/** アニメーションクリップ */
			std::unique_ptr<AnimationClip[]> m_animationClips;
			/** 今流しているアニメーション */
			Animation m_currentAnimation = Animation::Num;
			/** 門の中へ入りきって見えなくなったか（描かない） */
			bool m_isInsideGate = false;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
