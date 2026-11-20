#pragma once
#include "Source/Result/ResultParameter.h"
#include <memory>


/**
 * @file ResultPlayerModel.h
 * @brief 宝物の間のプレイヤー（見た目だけ。走ってきて、宝を見て跳ねて喜ぶ）
 */
namespace app
{
	namespace result
	{
		class CelebrationTimeline;


		/**
		 * @brief 宝物の間のプレイヤー（見た目だけ）
		 * @details ・ゲームのプレイヤーと同じモデルとアニメーション（Idle / Run / Jump）を使う。当たり判定や操作はない
		 *          ・位置とアニメーションは CelebrationTimeline の進み具合から決める
		 *              走っている間 … Run で右へ進む（止まる前はゆっくり）
		 *              跳ねている間 … Jump を最初から流して、上に放物線で跳ねる（跳ねるたびに Jump を流し直す）
		 *              それ以外     … Idle
		 *          ・喜ぶときは、カメラのほうへ少し振り向く
		 *          ・モデルの大きさ（4.75）と向き（右向き 90 度）は PlayerParameter / CharacterBase と同じ
		 */
		class ResultPlayerModel
		{
		public:
			ResultPlayerModel() = default;
			~ResultPlayerModel() = default;
			ResultPlayerModel(const ResultPlayerModel&) = delete;
			ResultPlayerModel& operator=(const ResultPlayerModel&) = delete;

			void Init(const TreasureRoomLayout& layout);
			/** 演出の進み具合に合わせて動かす（毎フレーム） */
			void Update(const CelebrationTimeline& timeline);
			void Draw(RenderContext& rc);

		private:
			/** アニメーションの番号（ANIMATION_DATA の並び） */
			enum class Animation
			{
				Idle,
				Run,
				Jump,
				Num,
			};

			/** アニメーションを変える（同じなら何もしない） */
			void Play(Animation animation);

		private:
			/** 置き方 */
			TreasureRoomLayout m_layout;
			/** モデル */
			ModelRender m_modelRender;
			/** アニメーションクリップ */
			std::unique_ptr<AnimationClip[]> m_animationClips;
			/** 今流しているアニメーション */
			Animation m_currentAnimation = Animation::Num;
			/** 最後に Jump を流し始めた跳ねる回数目（同じ跳ねで何度も流し直さないため） */
			int m_lastHopIndex = -1;
			/** 初期化済みか */
			bool m_isInited = false;
		};
	}
}
