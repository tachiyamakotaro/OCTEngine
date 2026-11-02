#pragma once


/**
 * @file GameplaySEEditor.h
 * @brief インゲームの SE（GameplaySEPlayer）を調整する ImGui（デバッグウィンドウの「Sound」の中身）
 */
namespace app
{
	class GameplaySEPlayer;


	/**
	 * @brief インゲームの SE を調整する ImGui
	 * @details ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
	 */
	class GameplaySEEditor
	{
	public:
		GameplaySEEditor() = default;
		~GameplaySEEditor() = default;

		/**
		 * @brief 調整項目と、最後に鳴らした SE を描く
		 * @param sePlayer 調整する SE
		 */
		void Draw(GameplaySEPlayer& sePlayer);

	private:
		/** 足音・着地・すべりの調整 */
		void DrawTiming(GameplaySEPlayer& sePlayer);
		/** 鳴らした数と、最後に鳴らした SE（ファイルがないときは注意を出す） */
		void DrawLastPlayed(const GameplaySEPlayer& sePlayer);
	};
}
