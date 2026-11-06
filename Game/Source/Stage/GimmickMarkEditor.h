#pragma once


/**
 * @file GimmickMarkEditor.h
 * @brief ギミックの印（電球マーク・歯車・レール・チェックポイントの旗）を調整する ImGui（デバッグウィンドウの「Gimmick Marks」の中身）
 */
namespace app
{
	namespace stage
	{
		class GimmickMarkRenderer;
		struct BulbStyle;
		struct GearStyle;
		struct RailStyle;
		struct FlagStyle;
		struct FlagWave;
		struct MarkPlacement;


		/**
		 * @brief ギミックの印を調整する ImGui
		 * @details ・GimmickMarkRenderer は ImGui を使わず、印を描くことだけをする。ImGui の項目はこのクラスにまとめる
		 *            （シーンに書くのは Draw を1行呼ぶだけでよい）
		 *          ・ウィンドウは作らない（Begin / End はしない）。デバッグウィンドウの中で呼ぶ
		 *          ・印ごとの項目は、その印の調整値（BulbStyle など）だけを受け取って描く
		 */
		class GimmickMarkEditor
		{
		public:
			GimmickMarkEditor() = default;
			~GimmickMarkEditor() = default;

			/**
			 * @brief 調整項目を描く
			 * @param renderer 調整する印の描画
			 */
			void Draw(GimmickMarkRenderer& renderer);

		private:
			/** 受光器の電球マーク */
			void DrawBulb(BulbStyle& style);
			/** 回せる反射鏡の歯車 */
			void DrawGear(GearStyle& style);
			/** 動かせる反射鏡のレール */
			void DrawRail(RailStyle& style);
			/** チェックポイントの旗 */
			void DrawFlag(FlagStyle& style);
			/**
			 * @brief 旗のはためき方1つぶん
			 * @param label 項目の名前（通る前・通ったあと）
			 */
			void DrawFlagWave(const char* label, FlagWave& wave);
			/** 印を置く奥行き */
			void DrawPlacement(MarkPlacement& placement);
		};
	}
}
