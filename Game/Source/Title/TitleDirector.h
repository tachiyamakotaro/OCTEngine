#pragma once
#include "Source/Title/EntranceTimeline.h"
#include "Source/Title/RuinEntrance.h"
#include "Source/Title/TitleCamera.h"
#include "Source/Title/TitleParameter.h"
#include "Source/Title/TitlePlayerModel.h"
#include "Source/Title/TitleSkyRenderer.h"


/**
 * @file TitleDirector.h
 * @brief タイトル画面の演出（遺跡の入口の 3D の景色と、プレイヤーが門へ入っていく流れ）をまとめる
 */
namespace app
{
	namespace title
	{
		/**
		 * @brief タイトル画面の演出をまとめる
		 * @details ・タイトルシーンは、このクラスとメニュー（ui::TitleMenu）を呼ぶだけにする
		 *          ・持っている物
		 *              EntranceTimeline … 時間の流れ（何秒に何が起きるか）
		 *              TitleCamera      … カメラ
		 *              TitleSkyRenderer … 夕暮れの空
		 *              RuinEntrance     … 地面・遺跡の建物・門（扉・門の光）
		 *              TitlePlayerModel … プレイヤー（走る・止まる・門のほうを向く）
		 *          ・始まると、プレイヤーが走ってきて門の少し手前で待つ
		 *          ・BeginEnter で、門の前まで走る → 門のほうを向く → 扉が開く（音も鳴らす）
		 *            IsEnterFinished が true になったら、シーンが次へ移ってよい
		 */
		class TitleDirector
		{
		public:
			TitleDirector() = default;
			~TitleDirector() = default;
			// Model を持っているのでコピー禁止
			TitleDirector(const TitleDirector&) = delete;
			TitleDirector& operator=(const TitleDirector&) = delete;

			void Init();
			/** 演出を進める（毎フレーム。シーンのフェード中も呼ぶ） */
			void Update(float deltaTime);
			/** 描く（空 → 地面・建物・門 → プレイヤー） */
			void Draw(RenderContext& rc);

			/** 門へ入り始める（START / CONTINUE を押したとき） */
			void BeginEnter();
			/** 門へ入る演出が終わったか（次のシーンへ移ってよいか） */
			bool IsEnterFinished() const { return m_timeline.IsEnterFinished(); }

		private:
			/** そのフレームに起きたことに合わせて、扉を開けて音を鳴らす */
			void HandleEvents(const EntranceTimeline::Events& events);

		private:
			/** 置き方 */
			RuinEntranceLayout m_layout;
			/** 時間の流れ */
			EntranceTimeline m_timeline;
			/** カメラ */
			TitleCamera m_camera;
			/** 夕暮れの空 */
			TitleSkyRenderer m_sky;
			/** 遺跡の入口（地面・建物・門） */
			RuinEntrance m_entrance;
			/** プレイヤー（見た目だけ） */
			TitlePlayerModel m_player;
		};
	}
}
