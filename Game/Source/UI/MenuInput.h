/**
 * @file MenuInput.h
 * @brief メニューのカーソル操作（十字キーと左スティックのどちらでも。スティックは倒した瞬間だけ）
 * @details メニューを開いている間、毎フレーム Update() を1回呼んでから IsUp() などを見る。
 *          メニューを開いたときは Reset() を呼ぶ（開いた瞬間にスティックを倒していても、勝手にカーソルが動かないように）。
 */
#pragma once


namespace app
{
	namespace ui
	{
		/**
		 * @brief メニューのカーソル操作
		 */
		class MenuInput
		{
		public:
			/** スティックを「倒したまま」の扱いにする（戻すまで次の入力にならない） */
			void Reset();
			/** スティックの入力を見る（毎フレーム1回） */
			void Update();

			bool IsUp() const;
			bool IsDown() const;
			bool IsLeft() const;
			bool IsRight() const;
			/** 決定（A） */
			bool IsDecide() const;
			/** 戻る（B） */
			bool IsCancel() const;


		private:
			/** スティックを上下・左右に倒しているか（倒した瞬間を見るため） */
			bool m_isStickUp = true;
			bool m_isStickDown = true;
			bool m_isStickLeft = true;
			bool m_isStickRight = true;
			/** 今フレーム倒した瞬間か */
			bool m_stickUpTrigger = false;
			bool m_stickDownTrigger = false;
			bool m_stickLeftTrigger = false;
			bool m_stickRightTrigger = false;
		};
	}
}
