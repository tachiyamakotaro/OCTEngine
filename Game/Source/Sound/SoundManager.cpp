/**
 * @file SoundManager.cpp
 * @brief サウンドの管理をするクラス
 */
#include "stdafx.h"
#include "SoundManager.h"
#include "Source/Core/Math/MathUtil.h"


namespace
{
	/** 最初の音量 */
	constexpr float DEFAULT_VOLUME = 0.5f;
	/** サウンドのゲームオブジェクトの名前 */
	constexpr const char* SOUND_GO_NAME = "se";
}


namespace app
{
	SoundManager* SoundManager::m_instance = nullptr;


	SoundManager::SoundManager()
		: m_masterVolume(DEFAULT_VOLUME)
		, m_bgmVolume(DEFAULT_VOLUME)
		, m_seVolume(DEFAULT_VOLUME)
	{
		// 波形データを読み込む（ファイルがなければ登録されないだけ）
		for (int kind = 0; kind < enSoundKind_Max; ++kind)
		{
			if (const char* path = GetSoundAssetPath(kind))
			{
				g_soundEngine->ResistWaveFileBank(kind, path);
			}
		}
	}


	SoundManager::~SoundManager()
	{
		StopAllSE();
		if (m_bgm != nullptr)
		{
			m_bgm->Stop();
			DeleteGO(m_bgm);
			m_bgm = nullptr;
		}
	}


	bool SoundManager::IsLoaded(const int kind) const
	{
		if (kind < 0 || kind >= enSoundKind_Max)
		{
			return false;
		}
		return static_cast<bool>(g_soundEngine->GetWaveFileBank().FindWaveFile(kind));
	}


	void SoundManager::Update()
	{
		// リクエストされたSEを鳴らす
		for (const SERequest& request : m_seRequests)
		{
			SoundSource* se = NewGO<SoundSource>(0, SOUND_GO_NAME);
			se->Init(request.kind);
			se->SetVolume(m_masterVolume * m_seVolume * request.volumeMagnification);
			se->Play(request.isLoop);
			// ループするSEだけ持っておく（ループしないSEは鳴り終わると SoundSource が自分で消える）
			if (request.isLoop)
			{
				// ポーズ中に鳴らされたループSEは、ポーズが解けるまで止めておく
				if (m_isLoopSEPaused)
				{
					se->Pause();
				}
				m_seList.emplace(request.handle, PlayingSE{ se, request.volumeMagnification });
			}
		}
		m_seRequests.clear();

		// BGMフェードアウトの進行
		if (m_isBgmFading && m_bgm != nullptr)
		{
			m_bgmFadeTimer += g_gameTime->GetFrameDeltaTime();
			ApplyBGMVolume();
			if (m_bgmFadeTimer >= m_bgmFadeDuration)
			{
				StopBGM();
			}
		}
	}


	void SoundManager::PlayBGM(const int kind, const float volumeMagnification)
	{
		m_bgmKind = kind;
		m_bgmVolumeMagnification = (volumeMagnification == INVALID_VOLUME) ? GetSoundDefaultVolume(kind) : volumeMagnification;
		// 新しいBGMを再生し直すので、進行中のフェードアウトは打ち切る
		m_isBgmFading = false;

		// 前のBGMは止めて消す（SoundSource は Init し直せないので作り直す）
		if (m_bgm != nullptr)
		{
			m_bgm->Stop();
			DeleteGO(m_bgm);
			m_bgm = nullptr;
		}
		if (!IsLoaded(kind))
		{
			return;
		}
		m_bgm = NewGO<SoundSource>(0, SOUND_GO_NAME);
		m_bgm->Init(kind);
		m_bgm->Play(true);
		ApplyBGMVolume();
	}


	void SoundManager::StopBGM()
	{
		m_isBgmFading = false;
		m_bgmKind = enSoundKind_None;
		if (m_bgm == nullptr)
		{
			return;
		}
		m_bgm->Stop();
		DeleteGO(m_bgm);
		m_bgm = nullptr;
	}


	void SoundManager::FadeOutBGM(const float duration)
	{
		if (m_bgm == nullptr)
		{
			return;
		}
		// 0秒以下が指定された場合は即座に停止する
		if (duration <= 0.0f)
		{
			StopBGM();
			return;
		}
		m_isBgmFading = true;
		m_bgmFadeTimer = 0.0f;
		m_bgmFadeDuration = duration;
	}


	SEHandle SoundManager::PlaySE(const int kind, const float volumeMagnification, const bool isLoop)
	{
		// ファイルがないサウンドは鳴らさない
		if (!IsLoaded(kind))
		{
			return INVALID_SE_HANDLE;
		}
		if (m_soundHandleCount == INVALID_SE_HANDLE)
		{
			K2_ASSERT(false, "サウンドの再生が多いです。\n");
			return INVALID_SE_HANDLE;
		}
		const SEHandle handle = m_soundHandleCount++;
		const float magnification = (volumeMagnification == INVALID_VOLUME) ? GetSoundDefaultVolume(kind) : volumeMagnification;
		m_seRequests.push_back({ kind, isLoop, handle, magnification });
		return handle;
	}


	void SoundManager::StopSE(const SEHandle handle)
	{
		if (handle == INVALID_SE_HANDLE)
		{
			return;
		}
		// まだ鳴らしていないリクエストなら取り消す
		for (auto request = m_seRequests.begin(); request != m_seRequests.end(); ++request)
		{
			if (request->handle == handle)
			{
				m_seRequests.erase(request);
				return;
			}
		}
		auto it = m_seList.find(handle);
		if (it != m_seList.end())
		{
			it->second.source->Stop();
			DeleteGO(it->second.source);
			m_seList.erase(it);
		}
	}


	bool SoundManager::IsPlayingSE(const SEHandle handle) const
	{
		if (handle == INVALID_SE_HANDLE)
		{
			return false;
		}
		if (m_seList.count(handle) > 0)
		{
			return true;
		}
		for (const SERequest& request : m_seRequests)
		{
			if (request.handle == handle)
			{
				return true;
			}
		}
		return false;
	}


	void SoundManager::SetPauseLoopSE(const bool isPause)
	{
		if (m_isLoopSEPaused == isPause)
		{
			return;
		}
		m_isLoopSEPaused = isPause;
		for (auto& se : m_seList)
		{
			if (isPause)
			{
				// SoundSource::Pause は止めた位置を覚えていて、Play で続きから鳴る
				se.second.source->Pause();
			}
			else
			{
				se.second.source->Play(true);
			}
		}
	}


	void SoundManager::StopAllSE()
	{
		for (auto& it : m_seList)
		{
			it.second.source->Stop();
			DeleteGO(it.second.source);
		}
		m_seList.clear();
		m_seRequests.clear();
	}


	void SoundManager::SetMasterVolume(const float volume)
	{
		m_masterVolume = math::Clamp01(volume);
		ApplyBGMVolume();
		ApplySEVolume();
	}


	void SoundManager::SetBGMVolume(const float volume)
	{
		m_bgmVolume = math::Clamp01(volume);
		ApplyBGMVolume();
	}


	void SoundManager::SetSEVolume(const float volume)
	{
		m_seVolume = math::Clamp01(volume);
		ApplySEVolume();
	}


	float SoundManager::CalcBGMVolume() const
	{
		float fade = 1.0f;
		if (m_isBgmFading && m_bgmFadeDuration > 0.0f)
		{
			fade = 1.0f - math::Clamp01(m_bgmFadeTimer / m_bgmFadeDuration);
		}
		return m_masterVolume * m_bgmVolume * m_bgmVolumeMagnification * fade;
	}


	void SoundManager::ApplyBGMVolume()
	{
		if (m_bgm != nullptr)
		{
			m_bgm->SetVolume(CalcBGMVolume());
		}
	}


	void SoundManager::ApplySEVolume()
	{
		for (auto& se : m_seList)
		{
			se.second.source->SetVolume(m_masterVolume * m_seVolume * se.second.volumeMagnification);
		}
	}
}
