/**
 * @file MenuGuide.h
 * @brief 画面の右下に出す操作の案内（「Ⓐ OK　Ⓑ BACK」または「Ⓑ BACK」）を、小さなウィンドウに入れて出す
 * @details 今まで StartStageMenu・StageSelectMenu・SoundOptionMenu がそれぞれ持っていた案内をまとめた。
 *          背景の上に直接文字を出さないように、帯なしの MenuWindow に入れる。文字には縁を付ける。
 *          右端をそろえて置くので、案内の数が変わってもウィンドウの大きさは自動で決まる。
 *          Place に UIGroupTransform を渡すと、案内のウィンドウの真ん中を中心に拡大縮小・濃さを変えて置ける
 *          （拡大率と濃さだけ使う。メニューのウィンドウと一緒に出し入れするため）。
 */
#pragma once
#include "Source/UI/MenuWindow.h"
#include "Source/UI/UISprite.h"


namespace app
{
	namespace ui
	{
		/**
		 * @brief 案内に出すボタン
		 */
		enum class MenuGuideButtons
		{
			Back,		//!< Ⓑ BACK だけ
			OkAndBack,	//!< Ⓐ OK　Ⓑ BACK
		};


		/**
		 * @brief 操作の案内
		 */
		class MenuGuide
		{
		public:
			MenuGuide();
			~MenuGuide();
			MenuGuide(const MenuGuide&) = delete;
			MenuGuide& operator=(const MenuGuide&) = delete;

			/**
			 * @brief 画像を読み込んで置く（1回だけ）
			 * @param buttons 出すボタン
			 */
			void Init(MenuGuideButtons buttons);
			/**
			 * @brief 拡大率・濃さをかけて置く（開閉のアニメーション中は毎フレーム呼ぶ）
			 * @param group 変形（拡大の中心は使わず、案内のウィンドウの真ん中にする）
			 */
			void Place(const UIGroupTransform& group);
			/** 描く */
			void Draw(RenderContext& rc);


		private:
			/** ボタンの絵と文字を右端から並べたときの位置と、まわりのウィンドウの形を決める */
			void Layout();


		private:
			/** Ⓐ OK を出すか */
			bool m_hasOk;
			/** 案内のウィンドウの形 */
			MenuWindowShape m_shape;
			/** ボタンの絵と文字の真ん中の X（Ⓐ・OK・Ⓑ・BACK の順。変形をかける前） */
			float m_buttonAX;
			float m_labelOkX;
			float m_buttonBX;
			float m_labelBackX;
			/** 後ろのウィンドウ（帯なし） */
			MenuWindow m_window;
			/** Ⓐ と OK */
			UISprite m_buttonA;
			UISprite m_labelOk;
			/** Ⓑ と BACK */
			UISprite m_buttonB;
			UISprite m_labelBack;
		};
	}
}
