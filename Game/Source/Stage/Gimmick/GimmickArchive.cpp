/**
 * @file GimmickArchive.cpp
 * @brief ギミックの値を JSON に書き出す・JSON から読み込む
 */
#include "stdafx.h"
#include "GimmickArchive.h"
#include <cmath>
#include "nlohmann/json.hpp"


namespace app
{
	namespace stage
	{
		namespace
		{
			/**
			 * @brief 書き出す小数を、小数点以下3桁に丸める
			 * @details float をそのまま書くと 0.15 が 0.15000000596046448 のようになり、手で読み書きしにくい
			 */
			double RoundForFile(const float value)
			{
				const double rounded = std::round(static_cast<double>(value) * 1000.0) / 1000.0;
				return rounded == 0.0 ? 0.0 : rounded;	// -0.0 を 0.0 にする
			}
		}


		GimmickArchive::GimmickArchive(nlohmann::ordered_json& node, const Mode mode)
			: m_node(node)
			, m_mode(mode)
		{}


		void GimmickArchive::Field(const char* key, float& value)
		{
			if (IsWriting())
			{
				m_node[key] = RoundForFile(value);
				return;
			}
			const auto it = m_node.find(key);
			if (it == m_node.end()) { return; }
			if (!it->is_number()) { Warn(key, "a number"); return; }
			value = it->get<float>();
		}


		void GimmickArchive::Field(const char* key, bool& value)
		{
			if (IsWriting())
			{
				m_node[key] = value;
				return;
			}
			const auto it = m_node.find(key);
			if (it == m_node.end()) { return; }
			if (!it->is_boolean()) { Warn(key, "true / false"); return; }
			value = it->get<bool>();
		}


		void GimmickArchive::Field(const char* key, int& value)
		{
			if (IsWriting())
			{
				m_node[key] = value;
				return;
			}
			const auto it = m_node.find(key);
			if (it == m_node.end()) { return; }
			if (!it->is_number()) { Warn(key, "a number"); return; }
			value = it->get<int>();
		}


		void GimmickArchive::Field(const char* key, std::string& value)
		{
			if (IsWriting())
			{
				m_node[key] = value;
				return;
			}
			const auto it = m_node.find(key);
			if (it == m_node.end()) { return; }
			if (!it->is_string()) { Warn(key, "a string"); return; }
			value = it->get<std::string>();
		}


		void GimmickArchive::Field(const char* key, math::Vec2& value)
		{
			if (IsWriting())
			{
				m_node[key] = { RoundForFile(value.x), RoundForFile(value.y) };
				return;
			}
			const auto it = m_node.find(key);
			if (it == m_node.end()) { return; }
			if (!it->is_array() || it->size() != 2 || !(*it)[0].is_number() || !(*it)[1].is_number())
			{
				Warn(key, "[x, y]");
				return;
			}
			value.x = (*it)[0].get<float>();
			value.y = (*it)[1].get<float>();
		}


		void GimmickArchive::Field(const char* key, std::vector<std::string>& values)
		{
			if (IsWriting())
			{
				m_node[key] = values;
				return;
			}
			const auto it = m_node.find(key);
			if (it == m_node.end()) { return; }
			if (!it->is_array()) { Warn(key, "[ \"name\", ... ]"); return; }
			std::vector<std::string> read;
			for (const auto& element : *it)
			{
				if (!element.is_string()) { Warn(key, "[ \"name\", ... ]"); return; }
				read.push_back(element.get<std::string>());
			}
			values = read;
		}


		void GimmickArchive::Rect(math::Rect& rect)
		{
			// 左下とサイズで書く（エディタの Position / Size と同じ）
			math::Vec2 position = rect.min;
			math::Vec2 size(rect.Width(), rect.Height());
			Field("position", position);
			Field("size", size);
			if (IsReading())
			{
				if (size.x < 1.0f) { size.x = 1.0f; }
				if (size.y < 1.0f) { size.y = 1.0f; }
				rect = math::Rect::FromMinSize(position.x, position.y, size.x, size.y);
			}
		}


		void GimmickArchive::Warn(const char* key, const char* expected)
		{
			m_warnings.push_back(std::string("\"") + key + "\" should be " + expected + " (ignored)");
		}
	}
}
