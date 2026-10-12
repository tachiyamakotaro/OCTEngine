/**
 * @file Fade.h
 * @brief 画面のフェード（真っ黒な画像を画面全体に出して、濃さを変える）
 * @details ProjectBeast の Fade をもとにしている。
 *          OCTEngine にはリソースを裏で読み込む仕組みがないので、ローディングサークルは外している。
 *          使い方：FadeOut(秒) で暗くし始め、IsFadeOutComplete() が true になったら真っ暗。
 *                  FadeIn(秒) で明るくし始め、IsFading() が false になったら終わり。
 *          Update() と Render() を毎フレーム呼ぶ（SceneManager が呼んでいる）。
 *          Render() はシーンの描画のあとに呼ぶと、シーンの画像の上に重なる。
 */
#pragma once


namespace app
{
	namespace core
	{
		class Fade
		{
		public:
			void Update();
			void Render(RenderContext& rc);


		public:
			/**
			 * @brief フェードアウトを開始する
			 * @param duration フェードアウトが完了するまでの時間
			 */
			void FadeOut(float duration);
			/**
			 * @brief フェードインを開始する
			 * @param duration フェードインが完了するまでの時間
			 */
			void FadeIn(float duration);


		public:
			/** 現在フェードをしているか */
			bool IsFading() const
			{
				return m_state != FadeState::None;
			}
			/** フェードアウトが完了したか（画面が完全に暗い状態） */
			bool IsFadeOutComplete() const
			{
				return m_state == FadeState::FadeOut && m_timer >= m_duration;
			}
			/** 今の暗さ（0=明るい 〜 1=真っ暗） */
			float GetAlpha() const;


		public:
			static void Create()
			{
				if (m_instance == nullptr)
				{
					m_instance = new Fade();
				}
			}
			static Fade& Get()
			{
				return *m_instance;
			}
			/** 作られているか */
			static bool IsCreated()
			{
				return m_instance != nullptr;
			}
			static void Delete()
			{
				if (m_instance != nullptr)
				{
					delete m_instance;
					m_instance = nullptr;
				}
			}

			/**
			 * @brief フェードイン中かどうか
			 * @return フェードイン中ならtrue、そうでなければfalse
			 */
			inline bool IsFadeIn() const { return m_isFadeIn; }
			/**
			 * @brief フェードアウト中かどうか
			 * @return フェードアウト中ならtrue、そうでなければfalse
			 */
			inline bool IsFadeOut() const { return m_isFadeOut; }


		private:
			Fade();
			~Fade();
			Fade(const Fade&) = delete;
			Fade& operator=(const Fade&) = delete;


		private:
			/** フェードの処理 */
			void FadeProcess();


		private:
			static Fade* m_instance;


		private:
			/** 背景スプライト（真っ黒な画像を画面いっぱいに出す） */
			SpriteRender m_fadeRender;


		private:
			/**
			 * @enum class FadeState
			 * @brief フェードの状態
			 */
			enum class FadeState
			{
				/** 何もしていない状態 */
				None,
				/** フェードイン */
				FadeIn,
				/** フェードアウト */
				FadeOut,
			};


		private:
			FadeState m_state = FadeState::None;

			/** フェードの経過時間 */
			float m_timer;

			/** フェードが完了するまでの時間 */
			float m_duration;

			/** フェードイン中かどうか */
			bool m_isFadeIn;
			/** フェードアウト中かどうか */
			bool m_isFadeOut;
		};
	}
}
