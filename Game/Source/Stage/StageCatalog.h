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
			/**
			 * @brief 制限時間（秒）
			 * @details 0（書かない）なら timer::DEFAULT_TIME_LIMIT（TimerParameter.h）、マイナスなら時間制限なし
			 */
			float timeLimitSeconds = 0.0f;
		};


		constexpr StageInfo STAGE_INFO_TABLE[] = {
			//{ "Stage01", "Assets/parameter/stage/Stage01_Object.json", "Assets/parameter/stage/Stage01_Gimmick.json", 180.0f },
			//{ "Stage02", "Assets/parameter/stage/Stage02_Object.json", "Assets/parameter/stage/Stage02_Gimmick.json", 180.0f },
			//// ギミックごとの確認用ステージ（時間制限の追加のときに消えていたので戻した）
			//{ "StageDark", "Assets/parameter/stage/StageDark_Object.json", "Assets/parameter/stage/StageDark_Gimmick.json", 180.0f },
			//{ "Stage03_Ice", "Assets/parameter/stage/Stage03_Object.json", "Assets/parameter/stage/Stage03_Gimmick.json", 180.0f },
			//{ "StageSolar", "Assets/parameter/stage/StageSolar_Object.json", "Assets/parameter/stage/StageSolar_Gimmick.json", 180.0f },
			//{ "StageMagnify", "Assets/parameter/stage/StageMagnify_Object.json", "Assets/parameter/stage/StageMagnify_Gimmick.json", 180.0f },
			//{ "ColorLight", "Assets/parameter/stage/ColorLight_Object.json", "Assets/parameter/stage/ColorLight_Gimmick.json", 180.0f },
			// 最終ステージ（3部屋。今まで作ったギミックを組み合わせる。1部屋 30秒〜1分、全体で長くて3分のつもりなので、制限時間は 4分）
			{ "StageFinal", "Assets/parameter/stage/StageFinal_Object.json", "Assets/parameter/stage/StageFinal_Gimmick.json", 300.0f },
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
		/**
		 * @brief 今遊ぶステージの制限時間（秒）
		 * @return 表に書いた値（0 なら timer::DEFAULT_TIME_LIMIT）。0 以下なら時間制限なし
		 */
		float GetCurrentStageTimeLimit();
	}
}
