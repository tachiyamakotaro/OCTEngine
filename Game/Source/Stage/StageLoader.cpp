/**
 * @file StageLoader.cpp
 * @brief ステージを JSON ファイルから読む・JSON ファイルに書く
 */
#include "stdafx.h"
#include "StageLoader.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include "nlohmann/json.hpp"
#include "Stage.h"
#include "StageCatalog.h"
#include "Gimmick/GimmickArchive.h"
#include "Gimmick/GimmickRegistry.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			using Json = nlohmann::ordered_json;


			/** ファイルを読んで JSON にする（失敗したら理由を messages に足して false） */
			bool ReadJsonFile(const char* path, Json& out, std::vector<std::string>& messages)
			{
				std::ifstream file(path);
				if (!file)
				{
					messages.push_back(std::string(path) + ": cannot open the file");
					return false;
				}
				try
				{
					// コメント（// や /* */）も書けるようにしておく
					out = Json::parse(file, nullptr, true, true);
				}
				catch (const Json::parse_error& error)
				{
					// error.what() に行と列が入っている
					messages.push_back(std::string(path) + ": " + error.what());
					return false;
				}
				if (!out.is_object())
				{
					messages.push_back(std::string(path) + ": the top must be { ... }");
					return false;
				}
				return true;
			}


			/**
			 * @brief ステージの設定（名前・開始位置・奥の鏡）を書き出す・読み込む
			 */
			void SerializeStageSettings(Json& node, Stage& stage, const GimmickArchive::Mode mode, const char* path, std::vector<std::string>& messages)
			{
				GimmickArchive archive(node, mode);
				std::string name = stage.GetName();
				math::Vec2 playerStart = stage.GetPlayerStart();
				archive.Field("name", name);
				archive.Field("playerStart", playerStart);

				BackMirror& backMirror = stage.GetBackMirror();
				Json& backMirrorNode = node["backMirror"];
				GimmickArchive backMirrorArchive(backMirrorNode, mode);
				backMirrorArchive.Field("enabled", backMirror.isEnabled);
				backMirrorArchive.Field("planeZ", backMirror.planeZ);
				backMirrorArchive.Rect(backMirror.frame);

				if (archive.IsReading())
				{
					stage.SetName(name);
					stage.SetPlayerStart(playerStart);
				}
				for (const std::string& warning : archive.GetWarnings()) { messages.push_back(std::string(path) + ": stage: " + warning); }
				for (const std::string& warning : backMirrorArchive.GetWarnings()) { messages.push_back(std::string(path) + ": backMirror: " + warning); }
			}


			/**
			 * @brief "gimmicks" の配列を読んで、ギミックを作って足す
			 */
			void ReadGimmicks(const Json& root, GimmickManager& gimmicks, const char* path, std::vector<std::string>& messages)
			{
				const auto list = root.find("gimmicks");
				if (list == root.end())
				{
					return;		// ギミックが1つもないステージ
				}
				if (!list->is_array())
				{
					messages.push_back(std::string(path) + ": \"gimmicks\" must be [ ... ]");
					return;
				}

				for (size_t i = 0; i < list->size(); ++i)
				{
					const std::string where = std::string(path) + ": gimmicks[" + std::to_string(i) + "]";
					const Json& item = (*list)[i];
					if (!item.is_object() || !item.contains("type") || !item["type"].is_string())
					{
						messages.push_back(where + ": needs \"type\" (skipped)");
						continue;
					}

					// "type" のクラス名から、登録表を使ってギミックを作る
					const std::string type = item["type"].get<std::string>();
					const GimmickRegistryEntry* entry = FindGimmickRegistryEntry(type.c_str());
					if (entry == nullptr)
					{
						messages.push_back(where + ": unknown type \"" + type + "\" (skipped)");
						continue;
					}
					std::unique_ptr<GimmickBase> gimmick = entry->create(math::Vec2::Zero());

					// 名前と、そのギミックの値を読む（書いていない値は初期値のまま）
					Json node = item;
					GimmickArchive archive(node, GimmickArchive::Mode::Read);
					archive.Field("name", gimmick->name);
					gimmick->Serialize(archive);
					for (const std::string& warning : archive.GetWarnings())
					{
						messages.push_back(where + " (" + gimmick->name + "): " + warning);
					}
					gimmicks.Add(std::move(gimmick));
				}
			}


			/** 1つのファイルを読む（"stage" があれば設定も読む） */
			bool ReadStageFile(const char* path, Stage& stage, std::vector<std::string>& messages)
			{
				Json root;
				if (!ReadJsonFile(path, root, messages))
				{
					return false;
				}
				if (root.contains("stage"))
				{
					SerializeStageSettings(root["stage"], stage, GimmickArchive::Mode::Read, path, messages);
				}
				ReadGimmicks(root, stage.GetGimmicks(), path, messages);
				return true;
			}


			/** JSON を1行の文字列にする（", " と ": " で区切って読みやすくする） */
			std::string DumpInline(const Json& node)
			{
				if (node.is_object())
				{
					std::string text = "{ ";
					bool isFirst = true;
					for (auto it = node.begin(); it != node.end(); ++it)
					{
						text += (isFirst ? "" : ", ") + Json(it.key()).dump() + ": " + DumpInline(it.value());
						isFirst = false;
					}
					return text + " }";
				}
				if (node.is_array())
				{
					std::string text = "[";
					for (size_t i = 0; i < node.size(); ++i)
					{
						text += (i > 0 ? ", " : "") + DumpInline(node[i]);
					}
					return text + "]";
				}
				return node.dump();
			}


			/**
			 * @brief 1つのファイルの中身を作る（ギミックは1行に1つ）
			 * @param isWriteSettings ステージの設定（"stage"）も書くか
			 * @param isWriteObject 地形のファイルに入れるギミックを書くか
			 * @param isWriteGimmick ギミックのファイルに入れるギミックを書くか
			 */
			std::string BuildFileText(const Stage& stage, const bool isWriteSettings, const bool isWriteObject, const bool isWriteGimmick)
			{
				// 書き出しでは値を読むだけで変えないので、const を外して Serialize を呼ぶ（読み込みと同じ関数を使うため）
				Stage& source = const_cast<Stage&>(stage);
				std::vector<std::string> unused;

				std::string text = "{\n";
				if (isWriteSettings)
				{
					Json settings = Json::object();
					SerializeStageSettings(settings, source, GimmickArchive::Mode::Write, "", unused);
					text += "\t\"stage\": " + DumpInline(settings) + ",\n";
				}
				text += "\t\"gimmicks\": [\n";
				bool isFirst = true;
				for (const std::unique_ptr<GimmickBase>& gimmick : source.GetGimmicks().GetAll())
				{
					const bool isObject = gimmick->GetFileKind() == StageFileKind::Object;
					if ((isObject && !isWriteObject) || (!isObject && !isWriteGimmick))
					{
						continue;
					}
					Json node = Json::object();
					node["type"] = gimmick->GetTypeName();
					GimmickArchive archive(node, GimmickArchive::Mode::Write);
					archive.Field("name", gimmick->name);
					gimmick->Serialize(archive);
					text += (isFirst ? "\t\t" : ",\n\t\t") + DumpInline(node);
					isFirst = false;
				}
				text += isFirst ? "\t]\n}\n" : "\n\t]\n}\n";
				return text;
			}


			/** 文字列をファイルに書く（フォルダがなければ作る） */
			bool WriteTextFile(const char* path, const std::string& text, std::vector<std::string>& messages)
			{
				std::error_code error;
				const std::filesystem::path filePath(path);
				if (filePath.has_parent_path())
				{
					std::filesystem::create_directories(filePath.parent_path(), error);
				}
				std::ofstream file(path, std::ios::binary);
				if (!file)
				{
					messages.push_back(std::string(path) + ": cannot write the file");
					return false;
				}
				file << text;
				return static_cast<bool>(file);
			}


			bool HasGimmickFile(const StageInfo& info)
			{
				return info.gimmickJsonPath != nullptr && info.gimmickJsonPath[0] != '\0';
			}
		}


		bool LoadStageFromJson(Stage& stage, const StageInfo& info, std::vector<std::string>& messages)
		{
			// いったん別のステージに読み、全部読めたら入れ替える（途中で失敗しても今のステージは壊さない）
			Stage loaded;
			if (!ReadStageFile(info.objectJsonPath, loaded, messages))
			{
				return false;
			}
			if (HasGimmickFile(info) && !ReadStageFile(info.gimmickJsonPath, loaded, messages))
			{
				return false;
			}

			stage.Clear();
			stage.SetName(loaded.GetName());
			stage.SetPlayerStart(loaded.GetPlayerStart());
			stage.GetBackMirror() = loaded.GetBackMirror();
			stage.GetGimmicks() = std::move(loaded.GetGimmicks());
			stage.ResetRuntimeState();
			return true;
		}


		bool SaveStageToJson(const Stage& stage, const StageInfo& info, std::vector<std::string>& messages)
		{
			if (!HasGimmickFile(info))
			{
				// ギミックのファイルがないステージは、全部を地形のファイルに書く
				return WriteTextFile(info.objectJsonPath, BuildFileText(stage, true, true, true), messages);
			}
			const bool isObjectWritten = WriteTextFile(info.objectJsonPath, BuildFileText(stage, true, true, false), messages);
			const bool isGimmickWritten = WriteTextFile(info.gimmickJsonPath, BuildFileText(stage, false, false, true), messages);
			return isObjectWritten && isGimmickWritten;
		}


		std::string StageToJsonText(const Stage& stage)
		{
			return BuildFileText(stage, true, true, true);
		}
	}
}
