/**
 * @file GimmickArchive.h
 * @brief ギミックの値を JSON に書き出す・JSON から読み込む
 * @details 1つの関数（GimmickBase::Serialize）で、書き出しと読み込みの両方を行う。
 *            void Door::Serialize(GimmickArchive& archive)
 *            {
 *                RectGimmick::Serialize(archive);                    // "position" と "size"
 *                archive.Field("openSpeed", openSpeed);              // 書き出し：openSpeed の値を JSON へ / 読み込み：JSON の値を openSpeed へ
 *            }
 *          読み込みのとき、JSON に書いていない値は今の値（コンストラクタの初期値）のまま変えない。
 *          そのため、あとからギミックに値を足しても、古い JSON はそのまま読める。
 *          型が違う値（数字のところに文字が書いてあるなど）は読まずに飛ばし、GetWarnings() に理由を残す。
 */
#pragma once
#include <string>
#include <vector>
#include "nlohmann/json_fwd.hpp"
#include "Source/Core/Math/Rect.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief ギミックの値の書き出し・読み込み
		 */
		class GimmickArchive
		{
		public:
			/** 書き出すか読み込むか */
			enum class Mode
			{
				Write,	//!< ギミックの値 → JSON
				Read,	//!< JSON → ギミックの値
			};


		public:
			/**
			 * @param node ギミック1つぶんの JSON（{ "type": ..., "name": ..., ... }）
			 * @param mode 書き出すか読み込むか
			 */
			GimmickArchive(nlohmann::ordered_json& node, Mode mode);

			bool IsWriting() const { return m_mode == Mode::Write; }
			bool IsReading() const { return m_mode == Mode::Read; }

			// ---- 値（key は JSON の名前。メンバ変数と同じ名前にしている） ----
			void Field(const char* key, float& value);
			void Field(const char* key, bool& value);
			void Field(const char* key, int& value);
			void Field(const char* key, std::string& value);
			/** [x, y] */
			void Field(const char* key, math::Vec2& value);
			/** [ "R1", "Switch1" ] */
			void Field(const char* key, std::vector<std::string>& values);
			/** 矩形（"position": [左下x, 左下y], "size": [幅, 高さ]） */
			void Rect(math::Rect& rect);

			/** 読み込みで飛ばした値の理由 */
			const std::vector<std::string>& GetWarnings() const { return m_warnings; }


		private:
			/** 読み込みで、型が違う値を飛ばしたことを記録する */
			void Warn(const char* key, const char* expected);


		private:
			nlohmann::ordered_json& m_node;
			Mode m_mode;
			std::vector<std::string> m_warnings;
		};
	}
}
