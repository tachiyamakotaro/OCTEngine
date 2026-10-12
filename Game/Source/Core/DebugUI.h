/**
 * @file DebugUI.h
 * @brief デバッグ用の表示と操作（ImGui のウィンドウ・エディタ・当たり判定の表示・F1/F2/F5）を使うか
 * @details 「見えないけれど触れる」状態を作らないために、表示と操作をまとめて切り替える。
 *          ・OFF の間は ImGui のウィンドウを1つも作らない（見えないウィンドウをマウスで触ってしまうことがない）
 *          ・OFF の間は F1（エディタ）/ F2（デバッグウィンドウ）/ F5（リスタート）も効かない
 *          ・最初は OFF。Debug ビルドは F4 で ON / OFF できる
 *          ・Release / Preview ビルド（NDEBUG）は ON にできない（F4 も効かない）
 */
#pragma once


namespace app
{
	namespace debug
	{
		/** @brief このビルドでデバッグ用の表示を使えるか（Release / Preview は使えない） */
		inline bool IsDebugUIAvailable()
		{
#ifdef NDEBUG
			return false;
#else
			return true;
#endif
		}


		/** @brief 今の状態（中身は1つだけ） */
		inline bool& DebugUIVisibleFlag()
		{
			static bool isVisible = false;
			return isVisible;
		}


		/** @brief デバッグ用の表示と操作を使っているか */
		inline bool IsDebugUIVisible()
		{
			return IsDebugUIAvailable() && DebugUIVisibleFlag();
		}


		/** @brief デバッグ用の表示と操作を使うか設定する（Release では何もしない） */
		inline void SetDebugUIVisible(const bool isVisible)
		{
			DebugUIVisibleFlag() = IsDebugUIAvailable() && isVisible;
		}
	}
}
