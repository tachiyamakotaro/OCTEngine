/**
 * @file StageGimmickTypes.h
 * @brief そのステージに置いてあるギミックの種類（JSON の "type"）の集まり
 * @details ステージを読み込まなくても、JSON から "type" だけを集めて作れる（StageLoader の LoadStageGimmickTypes）。
 *          ステージが始まる前の表示（StageIntroUI）が、どのアイコンを出すか決めるのに使う。
 */
#pragma once
#include <set>
#include <string>
#include <utility>


namespace app
{
	namespace stage
	{
		/**
		 * @brief そのステージに置いてあるギミックの種類の集まり
		 */
		class StageGimmickTypes
		{
		public:
			StageGimmickTypes() = default;
			explicit StageGimmickTypes(std::set<std::string> types)
				: m_types(std::move(types))
			{}

			/**
			 * @brief その種類のギミックが置いてあるか
			 * @param type JSON の "type"（"IceBlock" など）
			 */
			bool Contains(const char* type) const
			{
				return type != nullptr && m_types.count(type) > 0;
			}

		private:
			/** 置いてあるギミックの種類（JSON の "type"。同じ種類は1つにまとまる） */
			std::set<std::string> m_types;
		};
	}
}
