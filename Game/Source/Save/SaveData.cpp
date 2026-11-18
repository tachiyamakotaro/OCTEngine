/**
 * @file SaveData.cpp
 * @brief 続きから遊ぶための記録
 */
#include "stdafx.h"
#include "SaveData.h"
#include "nlohmann/json.hpp"
#include "Source/Stage/StageCatalog.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>


namespace app
{
	namespace save
	{
		namespace
		{
			using Json = nlohmann::ordered_json;

			/** 記録を置くフォルダとファイル（実行したフォルダからの場所） */
			const char* const SAVE_FOLDER = "Save";
			const char* const PROGRESS_PATH = "Save/progress.json";

			/** 表の中から名前でステージを探す（なければ -1） */
			int FindStageIndex(const std::string& name)
			{
				for (int i = 0; i < stage::STAGE_COUNT; ++i)
				{
					if (name == stage::STAGE_INFO_TABLE[i].name)
					{
						return i;
					}
				}
				return -1;
			}

			/** 表の番号として使えるか */
			bool IsValidStageIndex(const int index)
			{
				return index >= 0 && index < stage::STAGE_COUNT;
			}
		}


		bool HasProgress()
		{
			return LoadProgress().has_value();
		}


		std::optional<int> LoadProgress()
		{
			std::ifstream file(PROGRESS_PATH);
			if (!file)
			{
				return std::nullopt;
			}
			// 壊れたファイルでも止まらないように、例外を出さずに読む
			const Json root = Json::parse(file, nullptr, false);
			if (root.is_discarded() || !root.is_object())
			{
				return std::nullopt;
			}
			// 名前で探す（表の順番が変わっても同じステージ）。見つからなければ番号
			if (root.contains("stageName") && root["stageName"].is_string())
			{
				const int index = FindStageIndex(root["stageName"].get<std::string>());
				if (IsValidStageIndex(index))
				{
					return index;
				}
			}
			if (root.contains("stageIndex") && root["stageIndex"].is_number_integer())
			{
				const int index = root["stageIndex"].get<int>();
				if (IsValidStageIndex(index))
				{
					return index;
				}
			}
			return std::nullopt;
		}


		void SaveProgress(const int stageIndex)
		{
			if (!IsValidStageIndex(stageIndex))
			{
				return;
			}
			std::error_code error;
			std::filesystem::create_directories(SAVE_FOLDER, error);

			Json root;
			root["stageName"] = stage::STAGE_INFO_TABLE[stageIndex].name;
			root["stageIndex"] = stageIndex;
			std::ofstream file(PROGRESS_PATH, std::ios::trunc);
			if (!file)
			{
				OutputDebugStringA("[SaveData] 続きの記録を書けませんでした\n");
				return;
			}
			// 名前が UTF-8 でなくても止まらないように、読めない文字は置きかえて書く
			file << root.dump(2, ' ', false, Json::error_handler_t::replace);
		}


		void EraseProgress()
		{
			std::error_code error;
			std::filesystem::remove(PROGRESS_PATH, error);
		}
	}
}
