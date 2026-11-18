/**
 * @file StageIntroScene.cpp
 * @brief ステージが始まる前のシーン
 */
#include "stdafx.h"
#include "StageIntroScene.h"
#include "DebugScene.h"
#include "Source/Stage/StageCatalog.h"
#include "Source/Stage/StageLoader.h"


namespace
{
	/** 表示している時間（フェードで明るくなる時間も入る）。たったらインゲームへ */
	constexpr float SHOW_TIME = 2.2f;
	/** A ボタンで飛ばせるようになるまでの時間（前のシーンのボタンの押しっぱなしで飛ばないように） */
	constexpr float SKIP_WAIT_TIME = 0.5f;
	/** インゲームへ移るときのフェードの時間 */
	constexpr float NEXT_SCENE_FADE_TIME = 0.5f;
}


namespace app
{
	StageIntroScene::StageIntroScene()
		: m_ui()
		, m_timer(0.0f)
		, m_isNextScene(false)
	{}


	StageIntroScene::~StageIntroScene()
	{}


	bool StageIntroScene::Start()
	{
		// 最後のステージは「STAGE FINAL」、ほかは「STAGE 3」のように番号
		if (stage::IsCurrentStageFinal())
		{
			m_ui.InitFinal();
		}
		else
		{
			m_ui.InitNumber(stage::GetCurrentStageNumber());
		}
		// そのステージに置いてあるギミックのアイコン（JSON の "type" を見て選ぶ）
		m_ui.InitGimmickIcons(stage::LoadStageGimmickTypes(stage::GetCurrentStageInfo()));
		return true;
	}


	void StageIntroScene::Update()
	{
		m_timer += g_gameTime->GetFrameDeltaTime();
		const bool isSkipped = m_timer >= SKIP_WAIT_TIME && g_pad[0]->IsTrigger(enButtonA);
		if (m_timer >= SHOW_TIME || isSkipped)
		{
			m_isNextScene = true;
		}
	}


	void StageIntroScene::Render(RenderContext& rc)
	{
		m_ui.Render(rc);
	}


	bool StageIntroScene::RequestScene(uint32_t& id, float& fadeTime)
	{
		if (m_isNextScene)
		{
			// インゲーム（DebugScene の Start が今のステージの JSON を読む）
			id = DebugScene::ID();
			fadeTime = NEXT_SCENE_FADE_TIME;
			return true;
		}
		return false;
	}
}
