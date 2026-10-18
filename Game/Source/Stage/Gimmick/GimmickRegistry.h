/**
 * @file GimmickRegistry.h
 * @brief エディタで置けるギミックの一覧
 * @details 新しいギミックを作ったら、GimmickRegistry.cpp の一覧に1行足す。
 *          それだけで、エディタの「Add:」ボタン・物の一覧に出るようになる。
 */
#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "GimmickBase.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief エディタで置けるギミック1種類ぶん
		 */
		struct GimmickRegistryEntry
		{
			/** ギミックを作る関数（position は見取り図の中心。そこに置く） */
			using CreateFunction = std::function<std::unique_ptr<GimmickBase>(const math::Vec2& position)>;

			/** 追加ボタンの表示 */
			const char* buttonLabel;
			/** 新しく置いたときの名前の頭（"Door" → "Door1", "Door2" ...） */
			const char* namePrefix;
			/** 作る関数 */
			CreateFunction create;
			/** クラス名（create で1つ作って GetTypeName() から取る。一覧の見出しに使う） */
			const char* typeName;


			GimmickRegistryEntry(const char* buttonLabel, const char* namePrefix, CreateFunction create);
		};


		/** @brief エディタで置けるギミックの一覧（追加ボタンの並び順） */
		const std::vector<GimmickRegistryEntry>& GetGimmickRegistry();
		/** @brief クラス名から探す（見つからなければ nullptr） */
		const GimmickRegistryEntry* FindGimmickRegistryEntry(const char* typeName);
	}
}
