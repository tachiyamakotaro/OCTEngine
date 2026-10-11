/**
 * @file StageDebugOverlay.h
 * @brief ステージを3D画面の上に線と面で重ねて描く（デバッグ表示）
 * @details まだモデルがないギミック（扉・光・反射鏡・受光器・光の足場）や、
 *          当たり判定の箱を、ゲーム画面の上に ImGui の線で描く。
 *          アセットがそろうまではこれで遊べる。アセットがそろったら当たり判定の確認用に使う。
 *          奥の鏡があるときは、鏡に映った部屋（ギミックや光）も、鏡の奥の奥行きに薄く描く。
 *
 *          ワールド座標 → 画面座標の変換は外から渡す（Game でカメラを使って変換する）。
 *          こうしておくとエンジンに依存せず、テストでも動かせる。
 */
#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "Source/Core/Math/Vec2.h"
#include "imgui.h"


namespace app
{
	namespace actor
	{
		class CharacterBase;
	}


	namespace stage
	{
		class Stage;
		class MirrorInteractor;


		/**
		 * @brief ステージのデバッグ表示
		 */
		class StageDebugOverlay
		{
		public:
			/**
			 * @brief ワールド座標（XY平面の位置と奥行き z）を画面座標に変換する関数
			 * @return カメラの後ろなど、画面に出せないときは false
			 */
			using Projector = std::function<bool(const math::Vec2& world, float z, ImVec2& outScreen)>;


		public:
			/** 表示するか */
			bool isEnabled = true;
			/** 地形ブロックの面を塗るか（モデルで描いているなら OFF でもよい） */
			bool isFillBlocks = true;
			/** 奥の鏡に映った部屋も描くか */
			bool isDrawReflection = true;
			/** 扉・受光器・光の足場・反射鏡・光源・スイッチ・霧の範囲・鏡の足場・チェックポイントを描くか（モデルで描いているなら OFF でよい） */
			bool isDrawGimmicks = true;
			/** 光を描くか */
			bool isDrawLight = true;
			/** キャラクターの当たり判定とステート名を描くか */
			bool isDrawCharacters = true;


		public:
			/**
			 * @brief 描く（ImGui::NewFrame と ImGui::Render の間で呼ぶ）
			 * @param drawList 描き込み先（ふつうは ImGui::GetBackgroundDrawList()）
			 * @param stage ステージ
			 * @param interactor 反射鏡の操作（操作できる鏡に案内を出す。nullptr可）
			 * @param projector 座標変換
			 */
			void Draw(ImDrawList* drawList, const Stage& stage, const MirrorInteractor* interactor, const Projector& projector) const;
			/**
			 * @brief 表示するキャラクターを登録する
			 */
			void AddCharacter(const char* label, const actor::CharacterBase* character);
			/**
			 * @brief 表示の ON/OFF を切り替える ImGui の項目を出す
			 */
			void DrawSettingsUI();


		private:
			/** 表示するキャラクター */
			std::vector<std::pair<std::string, const actor::CharacterBase*>> m_characters;
		};
	}
}
