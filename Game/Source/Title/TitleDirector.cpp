/**
 * @file TitleDirector.cpp
 * @brief タイトル画面の演出をまとめる
 */
#include "stdafx.h"
#include "TitleDirector.h"
#include "Source/Sound/SoundManager.h"


namespace app
{
	namespace title
	{
		void TitleDirector::Init()
		{
			m_layout = RuinEntranceLayout();
			m_timeline.Init();
			m_camera.Init(m_layout);
			m_sky.Init(m_layout);
			m_entrance.Init(m_layout, m_timeline.GetDoorOpenTime());
			m_player.Init(m_layout);
		}


		void TitleDirector::Update(const float deltaTime)
		{
			HandleEvents(m_timeline.Update(deltaTime));

			m_camera.Apply();
			m_sky.Update(deltaTime);
			m_entrance.Update(deltaTime);
			m_player.Update(m_timeline);
			SceneLight::GetInstance()->Update();
		}


		void TitleDirector::Draw(RenderContext& rc)
		{
			// 空はいちばん奥。地面・建物・門・プレイヤーの順に登録する
			m_sky.Draw(rc);
			m_entrance.Draw(rc);
			m_player.Draw(rc);
		}


		void TitleDirector::BeginEnter()
		{
			m_timeline.BeginEnter();
		}


		void TitleDirector::HandleEvents(const EntranceTimeline::Events& events)
		{
			if (events.isDoorOpened)
			{
				m_entrance.OpenDoor();
				SoundManager::Get().PlaySE(enSoundKind_DoorOpen);
			}
		}
	}
}
