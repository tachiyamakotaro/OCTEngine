/**
 * @file StageLoader.h
 * @brief ステージを JSON ファイルから読む・JSON ファイルに書く
 * @details ファイルの形（地形のファイル。ギミックのファイルは "stage" がないだけで同じ形）
 *            {
 *              "stage": { "name": "Stage01", "playerStart": [230.0, 0.0],
 *                         "backMirror": { "enabled": true, "planeZ": 150.0, "position": [-100.0, 0.0], "size": [6900.0, 3100.0] } },
 *              "gimmicks": [
 *                { "type": "StageBlock", "name": "Floor", "position": [-200.0, -100.0], "size": [1700.0, 100.0] },
 *                { "type": "Door", "name": "Door1", "position": [900.0, 0.0], "size": [40.0, 300.0], "requiredReceivers": ["R1"] }
 *              ]
 *            }
 *          ・"type" はギミックのクラス名。GimmickRegistry.cpp に登録してあるクラスなら何でも置ける。
 *          ・知らない "type" は飛ばして、メッセージに残す（ほかのギミックはそのまま読む）。
 *          ・書いていない値は、そのクラスの初期値になる。
 *          ・読み込みに失敗したとき（ファイルがない・文法の間違い）は、今のステージを変えない。
 */
#pragma once
#include <string>
#include <vector>


namespace app
{
	namespace stage
	{
		class Stage;
		struct StageInfo;


		/**
		 * @brief ステージを JSON から読む（地形のファイル → ギミックのファイルの順）
		 * @param stage 読み込み先（失敗したら変えない）
		 * @param info どのファイルを読むか
		 * @param messages 読み込みで気づいたこと（知らない type・型の違う値・失敗の理由）を足していく
		 * @return 読めたら true
		 */
		bool LoadStageFromJson(Stage& stage, const StageInfo& info, std::vector<std::string>& messages);
		/**
		 * @brief ステージを JSON に書く（地形ブロックは地形のファイル、ほかはギミックのファイル）
		 * @return 書けたら true
		 */
		bool SaveStageToJson(const Stage& stage, const StageInfo& info, std::vector<std::string>& messages);
		/**
		 * @brief ステージを JSON の文字列にする（エディタの「変更があったか」の判定に使う）
		 */
		std::string StageToJsonText(const Stage& stage);
	}
}
