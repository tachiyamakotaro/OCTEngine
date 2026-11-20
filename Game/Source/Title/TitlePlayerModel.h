#pragma once
#include "Source/Title/TitleParameter.h"
#include <memory>


/**
 * @file TitlePlayerModel.h
 * @brief タイトル画面のプレイヤー（見た目だけ。遺跡の門の少し手前まで走ってきて待ち、START / CONTINUE で門の前へ行って門のほうを向く）
 */
namespace app
{
	namespace title
	{
		class EntranceTimeline;


		/**
		 * @brief タイトル画面のプレイヤー（見た目だけ）
		 * @details ・ゲームのプレイヤーと同じモデルとアニメーション（Idle / Run）を使う。当たり判定や操作はない
		 *          ・位置とアニメーションは EntranceTimeline の進み具合から決める
		 *              走っている間 … Run で右へ進む（始まったとき：左の外 → 門の少し手前、押したとき：門の少し手前 → 門の前）
		 *              それ以外     … Idle
		 *          ・門の前に着いたら、門のほう（奥）へ向きを変える（背中が見えて「これから入っていく」ように見せる）
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
			/** 演出の進み具合に合わせて動かす（毎フレーム） */
			void Update(const EntranceTimeline& timeline);
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
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
