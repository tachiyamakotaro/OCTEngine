/**
 * @file StageCatalog.h
 * @brief ステージの一覧（どのステージがどの JSON ファイルを読むか）
 * @details ステージを増やすときは、STAGE_INFO_TABLE に1行足して、2つの JSON ファイルを作るだけでよい。
 *          ・地形のファイル   … ステージの設定（名前・開始位置・奥の鏡）と地形ブロック
 *          ・ギミックのファイル … それ以外のギミック（光源・鏡・扉・透明の足場 ...）
 *          どのギミックを置くかはステージごとに自由（JSON に書いてある物だけが作られる）。
 *          中身は StageEditor の「Save JSON」で書き出せる。手で書き換えてもよい。
 */
#pragma once
#include <iterator>


namespace app
{
	namespace stage
	{
		/**
		 * @brief 1ステージぶんの情報
		 */
		struct StageInfo
		{
			/** ステージの名前（一覧の表示・ログ用） */
			const char* name;
			/** 地形のファイル（ステージの設定と地形ブロック） */
			const char* objectJsonPath;
			/** ギミックのファイル（nullptr ならギミックなし） */
			const char* gimmickJsonPath;
		};


		constexpr StageInfo STAGE_INFO_TABLE[] = {
			{ "Stage01", "Assets/parameter/stage/Stage01_Object.json", "Assets/parameter/stage/Stage01_Gimmick.json" },
			{ "Stage02", "Assets/parameter/stage/Stage02_Object.json", "Assets/parameter/stage/Stage02_Gimmick.json" },
		};

		/** ステージの数 */
		constexpr int STAGE_COUNT = static_cast<int>(std::size(STAGE_INFO_TABLE));
		/**
		 * @brief タイトルから START したときに遊ぶステージ（STAGE_INFO_TABLE の番号）
		 * @details 手鏡の確認のために、今は Stage02（1）から始める。本番は 0 に戻す。
		 */
		constexpr int FIRST_STAGE_INDEX = 0;
		static_assert(FIRST_STAGE_INDEX >= 0 && FIRST_STAGE_INDEX < STAGE_COUNT, "FIRST_STAGE_INDEX が表の範囲の外");


		/** @brief 今遊ぶステージの番号（タイトルやステージ選択で決める） */
		int GetCurrentStageIndex();
		/** @brief 今遊ぶステージの番号を決める（範囲外なら一番近いステージにする） */
		void SetCurrentStageIndex(int index);
		/** @brief 今遊ぶステージの情報 */
		const StageInfo& GetCurrentStageInfo();
	}
}
