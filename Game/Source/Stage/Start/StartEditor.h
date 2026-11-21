#pragma once


/**
 * @file StartEditor.h
 * @brief ステージ開始の演出を調整する ImGui（デバッグウィンドウの「Stage Start」の中身）
 */
namespace app
{
	namespace stage
	{
		class StartSequence;


		/**
		 * @brief ステージ開始の演出を調整する ImGui
		 * @details ・StartSequence は ImGui を使わない。ImGui の項目はこのクラスにまとめる
		 *          ・ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 *          ・演出をもう一度見るボタン（Replay Start）は、シーンの処理が要るので「Stage」の所にある
		 */
		class StartEditor
		{
		public:
			StartEditor() = default;
			~StartEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param sequence 調整するステージ開始の演出
			 */
			void Draw(StartSequence& sequence);
		};
	}
}
