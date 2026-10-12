/**
 * @file SoundManager.h
 * @brief サウンドの管理をするクラス
 * @details ProjectBeast の SoundManager をもとにしている（C++14 で動くように std::optional / std::clamp を使わない形にした）。
 *          ・BGM は1つだけ、必ずループ
 *          ・SE はリクエストしておき、Update でまとめて鳴らす
 *            ループしない SE は鳴らしっぱなし（エンジンの SoundSource は鳴り終わると自分で DeleteGO するので、
 *            ポインタを持ち続けると消えたあとに触ってしまう）。止めたり音量を変えたりできるのはループする SE だけ
 *          ・音量は 全体 × 種類（BGM / SE）× 個別の倍率（倍率を指定しなければ Types.h の GetSoundDefaultVolume）
 *          ・ファイルが見つからなかったサウンドは鳴らさない（エンジンの SoundSource は、波形がないまま
 *            音量を変えたり止めたりすると落ちるため、ここで弾いている）
 */
#pragma once
#include <map>
#include <vector>
#include "Source/Sound/SoundHandle.h"
#include "Source/Sound/Types.h"


namespace app
{
	/** 無効な音量値 */
	static constexpr float INVALID_VOLUME = -1.0f;
	static constexpr float DEFAULT_VOLUME_MAGNIFICATION = 1.0f;


	/**
	 * サウンドを管理するクラス
	 */
	class SoundManager
	{
	private:
		/**
		 * SE再生のリクエスト
		 */
		struct SERequest
		{
			int kind;
			bool isLoop;
			SEHandle handle;
			float volumeMagnification;
		};


		/**
		 * 再生中のSE
		 */
		struct PlayingSE
		{
			SoundSource* source;
			float volumeMagnification;
		};


	public:
		/**
		 * @brief 更新処理（リクエストされたSEを鳴らす、BGMのフェードアウト）
		 * @details 毎フレーム呼ぶ（Game::Update で呼んでいる）
		 */
		void Update();


	public:
		/** BGMの再生（BGMは常に1つ・必ずループ） */
		void PlayBGM(int kind, float volumeMagnification = INVALID_VOLUME);
		/** BGMの停止 */
		void StopBGM();
		/** BGMを指定時間かけて徐々にフェードアウトし、完了後に停止する */
		void FadeOutBGM(float duration);

		/**
		 * SEの再生のリクエスト
		 * NOTE: 次の Update でまとめて再生される
		 * @param volumeMagnification 音量の倍率（指定しなければ GetSoundDefaultVolume(kind)）
		 * @param isLoop ループするか（ループする SE は StopSE で止めること）
		 */
		SEHandle PlaySE(int kind, float volumeMagnification = INVALID_VOLUME, bool isLoop = false);
		/** SEの停止（ループするSEだけ） */
		void StopSE(SEHandle handle);
		/** 全てのループするSEを停止 */
		void StopAllSE();
		/** @brief SE が鳴っているか（ループする SE だけ分かる。鳴らす前のリクエストも鳴っている扱い） */
		bool IsPlayingSE(SEHandle handle) const;
		/**
		 * @brief ループする SE を一時停止する / 続きから鳴らす（ポーズ画面で使う）
		 * @details ループしない SE は短いので、そのまま鳴り終わらせる
		 */
		void SetPauseLoopSE(bool isPause);
		/** @brief ループする SE を一時停止しているか */
		bool IsLoopSEPaused() const { return m_isLoopSEPaused; }


	public:
		/** BGMが再生中か */
		bool IsPlayingBGM() const { return m_bgm != nullptr && m_bgm->IsPlaying(); }
		/** 今のBGMの種類（鳴らしていなければ enSoundKind_None） */
		int GetBGMKind() const { return m_bgmKind; }
		/** 再生中のループするSEの数 */
		size_t GetPlayingLoopSECount() const { return m_seList.size(); }
		/** サウンドのファイルを読み込めたか */
		bool IsLoaded(int kind) const;


	public:
		/** 全体の音量設定（0〜1） */
		void SetMasterVolume(float volume);
		/** BGMの音量設定（0〜1） */
		void SetBGMVolume(float volume);
		/** SEの音量設定（0〜1） */
		void SetSEVolume(float volume);

		float GetMasterVolume() const { return m_masterVolume; }
		float GetBGMVolume() const { return m_bgmVolume; }
		float GetSEVolume() const { return m_seVolume; }


	public:
		/** インスタンスを作成（Game::Start で呼ぶ） */
		static void CreateInstance()
		{
			if (m_instance == nullptr)
			{
				m_instance = new SoundManager();
			}
		}
		/** インスタンスを取得 */
		static SoundManager& Get() { return *m_instance; }
		/** 作られているか */
		static bool IsCreated() { return m_instance != nullptr; }
		/** インスタンスを削除 */
		static void DestroyInstance()
		{
			delete m_instance;
			m_instance = nullptr;
		}


	private:
		SoundManager();
		~SoundManager();
		SoundManager(const SoundManager&) = delete;
		SoundManager& operator=(const SoundManager&) = delete;

		void ApplyBGMVolume();
		void ApplySEVolume();
		/** BGMの今の音量（フェードアウト中はだんだん小さく） */
		float CalcBGMVolume() const;


	private:
		/** BGM用のサウンドソース */
		SoundSource* m_bgm = nullptr;
		/** 今のBGMの種類 */
		int m_bgmKind = enSoundKind_None;
		/** 今のBGMの音量の倍率 */
		float m_bgmVolumeMagnification = DEFAULT_VOLUME_MAGNIFICATION;

		/** 再生中のループするSE */
		std::map<SEHandle, PlayingSE> m_seList;
		/** SE再生のリクエスト */
		std::vector<SERequest> m_seRequests;
		/** 次に使うハンドル */
		SEHandle m_soundHandleCount = 0;

		/** BGMフェードアウト中かどうか */
		bool m_isBgmFading = false;
		/** ループする SE を一時停止しているか */
		bool m_isLoopSEPaused = false;
		/** BGMフェードアウトの経過時間 */
		float m_bgmFadeTimer = 0.0f;
		/** BGMフェードアウトにかける時間（秒） */
		float m_bgmFadeDuration = 0.0f;

		/** 全体の音量 */
		float m_masterVolume;
		/** BGMの音量 */
		float m_bgmVolume;
		/** SEの音量 */
		float m_seVolume;

		/** シングルトンインスタンス */
		static SoundManager* m_instance;
	};
}
