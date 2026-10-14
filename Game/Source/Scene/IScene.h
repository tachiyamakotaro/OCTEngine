/**
 * @file IScene.h
 * @brief シーンのインターフェース
 * @details シーンを作るときは IScene を継承し、クラスの最初に appScene(クラス名); を書く。
 *          appScene はシーンの番号 ID() を作る（クラス名の文字列から計算するので、重ならない）。
 *          作ったシーンは SceneManager のコンストラクタで AddSceneMap<クラス名>() に登録する。
 */
#pragma once
#include <cstdint>


namespace app
{
	/** シーンの番号がないことを表す値 */
	constexpr uint32_t INVALID_SCENE_ID = 0;


	/**
	 * @brief 文字列から 32bit の番号を作る（FNV-1a。コンパイル時に計算できる）
	 */
	constexpr uint32_t SceneHash(const char* text)
	{
		uint32_t hash = 2166136261u;
		for (; *text != '\0'; ++text)
		{
			hash ^= static_cast<uint8_t>(*text);
			hash *= 16777619u;
		}
		return hash;
	}


	/**
	 * @brief シーンのクラスの最初に書くマクロ（シーンの番号と名前を作る）
	 */
#define appScene(className) \
	public: \
		static constexpr uint32_t ID() { return ::app::SceneHash(#className); } \
		const char* GetName() const override { return #className; } \
	private: \
		static_assert(true, "")


	/**
	 * @brief シーンのインターフェース
	 */
	class IScene
	{
	public:
		virtual ~IScene() = default;

		/** シーンが作られたときに1回呼ばれる（モデルや画像の読み込み） */
		virtual bool Start() = 0;
		/** 毎フレームの更新 */
		virtual void Update() = 0;
		/** フェードアウト中・ポーズ中の更新（操作は受け付けず、演出だけ動かしたいとき用。ゲームを止めたいなら何もしない） */
		virtual void PauseUpdate() {}
		/** 描画 */
		virtual void Render(RenderContext& rc) = 0;
		/**
		 * @brief 次のシーンへ移りたいか
		 * @param id 移りたいシーンの番号（〇〇Scene::ID()）
		 * @param fadeTime フェードの時間（秒。暗くなるのとまた明るくなるのに、それぞれこの時間をかける）
		 * @return 移りたいなら true
		 */
		virtual bool RequestScene(uint32_t& id, float& fadeTime) = 0;
		/**
		 * @brief ゲームを終わりたいか（タイトルの QUIT など）
		 * @param fadeTime 画面を暗くする時間（秒。暗くなりきったらゲームを終わる）
		 * @return 終わりたいなら true
		 */
		virtual bool RequestQuit(float& fadeTime) { (void)fadeTime; return false; }
		/** ポーズできるシーンか（インゲームだけ true にする） */
		virtual bool CanPause() const { return false; }
		/** 読み込みが終わったか（時間のかかる読み込みを分けて行うシーン用。ふつうは true のまま） */
		virtual bool IsLoaded() const { return true; }
		/** シーンの名前（appScene が作る） */
		virtual const char* GetName() const = 0;
	};
}
