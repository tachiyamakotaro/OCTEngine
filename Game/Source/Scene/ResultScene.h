/**
 * @file ResultScene.h
 * @brief リザルトシーン（最後のステージをクリアしたあとの宝物の間。宝を見つけて喜ぶ演出のあと、A ボタンでタイトルへ）
 */
#pragma once
#include "Source/Core/IrisWipe.h"
#include "Source/Result/CelebrationTimeline.h"
#include "Source/Result/ResultParameter.h"
#include "Source/Result/ResultPlayerModel.h"
#include "Source/Result/TreasureRenderer.h"
#include "Source/Scene/IScene.h"
#include "Source/Stage/Background/BackgroundRenderer.h"
#include "Source/Stage/Stage.h"
#include "Source/Stage/StageRenderer.h"
#include "Source/UI/ResultUI.h"


namespace app
{
	/**
	 * @brief リザルトシーン（宝物の間）
	 * @details ・最後のステージの丸が閉じたあと、真っ暗から丸が開いて宝物の間になる
	 *          ・プレイヤーが走ってきて止まり、宝箱が開いて光の宝石が浮かび、2回跳ねて喜ぶ。GAME CLEAR が出る
	 *            （時間の流れは result::CelebrationTimeline、宝箱は result::TreasureRenderer、
	 *              プレイヤーは result::ResultPlayerModel、2D の表示は result::ResultUI）
	 *          ・部屋は地形ブロックで作る（result::BuildTreasureRoom）ので、地形と奥の背景はインゲームと同じ物で描く
	 *          ・A ボタン：演出の途中なら最後まで飛ばす。案内が出たらタイトルへ
	 */
	class ResultScene : public IScene
	{
		appScene(ResultScene);
	public:
		ResultScene();
		~ResultScene() override;
		bool Start() override;
		void Update() override;
		void Render(RenderContext& rc) override;
		bool RequestScene(uint32_t& id, float& fadeTime) override;

	private:
		/** カメラを置く（平行投影。演出が進むほど少し寄る） */
		void UpdateCamera();
		/** そのフレームに起きたことに合わせて音を鳴らす */
		void PlaySounds(const result::CelebrationTimeline::Events& events);

	private:
		/** 置き方 */
		result::TreasureRoomLayout m_layout;
		/** 時間の流れ */
		result::CelebrationTimeline m_timeline;
		/** 部屋の地形 */
		stage::Stage m_room;
		/** 地形の描画 */
		stage::StageRenderer m_stageRenderer;
		/** 奥の背景（回廊と松明） */
		stage::BackgroundRenderer m_backgroundRenderer;
		/** 宝箱・宝石 */
		result::TreasureRenderer m_treasure;
		/** プレイヤー（見た目だけ） */
		result::ResultPlayerModel m_player;
		/** GAME CLEAR・ボタンの案内 */
		result::ResultUI m_ui;
		/** 丸く開くフェード */
		core::IrisWipe m_irisWipe;
		/** シーンが始まってからの時間（すぐに A を受け付けないため） */
		float m_timer;
		/** 次のシーンへ移るか */
		bool m_isNextScene;
	};
}
