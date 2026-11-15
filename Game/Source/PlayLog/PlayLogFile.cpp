/**
 * @file PlayLogFile.cpp
 * @brief プレイログを書き出すファイル
 */
#include "stdafx.h"
#include "PlayLogFile.h"
#include <filesystem>
#include <fstream>


namespace app
{
	namespace playlog
	{
		namespace
		{
			/** ログを置くフォルダ（実行したフォルダからの場所） */
			const char* const LOG_FOLDER = "PlayLog";
			/** ファイルの拡張子 */
			const char* const LOG_EXTENSION = ".txt";
			/** UTF-8 の BOM（メモ帳・Excel に UTF-8 だと伝える印） */
			const char UTF8_BOM[] = "\xEF\xBB\xBF";
		}


		bool PlayLogFile::SaveAsNewFile(const std::tm& startTime, const std::string& text) const
		{
			// フォルダがなければ作る（失敗しても、下でファイルが開けなければ false を返す）
			std::error_code error;
			std::filesystem::create_directories(LOG_FOLDER, error);

			const std::string path = MakeNewPath(startTime);
			std::ofstream file(path, std::ios::binary | std::ios::trunc);
			if (!file)
			{
				OutputDebugStringA(("[PlayLog] ファイルを作れませんでした: " + path + "\n").c_str());
				return false;
			}
			file.write(UTF8_BOM, sizeof(UTF8_BOM) - 1);
			file.write(text.data(), static_cast<std::streamsize>(text.size()));
			OutputDebugStringA(("[PlayLog] 書き出しました: " + path + "\n").c_str());
			return static_cast<bool>(file);
		}


		std::string PlayLogFile::MakeNewPath(const std::tm& startTime) const
		{
			char fileName[64] = {};
			std::strftime(fileName, sizeof(fileName), "PlayLog_%Y%m%d_%H%M%S", &startTime);
			const std::string basePath = std::string(LOG_FOLDER) + "/" + fileName;

			// 同じ秒に遊び始めたプレイがあっても上書きしないように、番号を付ける
			std::string path = basePath + LOG_EXTENSION;
			for (int number = 2; std::filesystem::exists(path); ++number)
			{
				path = basePath + "_" + std::to_string(number) + LOG_EXTENSION;
			}
			return path;
		}
	}
}
