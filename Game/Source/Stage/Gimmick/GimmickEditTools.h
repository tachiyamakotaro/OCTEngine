/**
 * @file GimmickEditTools.h
 * @brief ギミックがエディタ用の処理（見取り図・プロパティ）を書くための道具
 * @details ・EditorCanvas      … 見取り図に矩形・線・点・文字を描く（ワールド座標のまま渡せる）
 *          ・GimmickInspector  … プロパティ欄でよく使う入力欄（矩形・角度・信号の選択）
 *          ImGui に日本語フォントを読み込んでいないため、画面上の表示は英語にしている。
 */
#pragma once
#include <string>
#include <vector>
#include "imgui.h"
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief エディタの見取り図に描く
		 */
		class EditorCanvas
		{
		public:
			/** 選択中の物の色 */
			static constexpr ImU32 COLOR_SELECTED = IM_COL32(255, 90, 90, 255);
			/** 名前の文字の色 */
			static constexpr ImU32 COLOR_LABEL = IM_COL32(255, 255, 255, 170);


		public:
			/**
			 * @param drawList 描き込む先
			 * @param viewCenter 見取り図の中心に映すワールド座標
			 * @param zoom 1ワールド単位あたりのピクセル
			 * @param screenCenter 見取り図の画面上の中心
			 */
			EditorCanvas(ImDrawList* drawList, const math::Vec2& viewCenter, float zoom, const ImVec2& screenCenter);

			/** ワールド座標 → 画面座標 */
			ImVec2 WorldToScreen(const math::Vec2& world) const;
			/** 画面の長さ（ピクセル）→ ワールドの長さ */
			float PixelsToWorld(float pixels) const { return pixels / m_zoom; }
			/** 矩形（色が 0 なら描かない） */
			void Rect(const math::Rect& rect, ImU32 fillColor, ImU32 outlineColor) const;
			/** 線 */
			void Line(const math::Vec2& a, const math::Vec2& b, ImU32 color, float thickness = 1.0f) const;
			/** 塗った円（半径はピクセル） */
			void Dot(const math::Vec2& center, float radiusPixels, ImU32 color) const;
			/** 文字（位置の右下に描く） */
			void Label(const math::Vec2& world, const std::string& text) const;
			/** 選択中なら選択色、そうでなければ color */
			static ImU32 Pick(const bool isSelected, const ImU32 color) { return isSelected ? COLOR_SELECTED : color; }


		private:
			ImDrawList* m_drawList;
			math::Vec2 m_viewCenter;
			float m_zoom;
			ImVec2 m_screenCenter;
		};


		/**
		 * @brief プロパティ欄の入力欄
		 */
		class GimmickInspector
		{
		public:
			/** 信号を出す物（受光器・スイッチ）の名前。エディタが毎フレーム入れる */
			std::vector<std::string> signalNames;


		public:
			/** 矩形の欄（左下とサイズ）。変更されたら true */
			bool EditRect(math::Rect& rect, const char* positionLabel = "Position", const char* sizeLabel = "Size");
			/** 角度の欄（±ボタン付き） */
			void EditAngle(const char* label, float& angleDeg, float stepDeg);
			/** 信号を1つ選ぶコンボボックス（空も選べる） */
			void SignalCombo(const char* label, std::string& signal);
			/** 信号をいくつでも選べるチェックボックス（扉の「全部ONで開く」など） */
			void SignalCheckboxes(std::vector<std::string>& signals);
		};
	}
}
