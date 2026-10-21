/**
 * @file LightColor.cpp
 * @brief 光の色
 */
#include "stdafx.h"
#include "LightColor.h"
#include "imgui.h"
#include "Gimmick/GimmickArchive.h"


namespace app
{
	namespace stage
	{
		namespace
		{
			/**
			 * @brief 色ごとの情報（LightColor の値の順に並べる）
			 */
			struct LightColorInfo
			{
				const char* name;
				/** 描くときの色 */
				float rgb[3];
				/** エディタの色 */
				uint32_t editorColor;
			};

			// エディタの色は IM_COL32(r, g, b, 255) と同じ並び（0xAABBGGRR）
			constexpr LightColorInfo LIGHT_COLOR_INFOS[] = {
				{ "None",    { 0.0f,  0.0f,  0.0f  }, 0xFF505050 },
				{ "Red",     { 1.0f,  0.18f, 0.12f }, 0xFF3C3CFF },
				{ "Green",   { 0.2f,  1.0f,  0.25f }, 0xFF46FF46 },
				{ "Yellow",  { 1.0f,  0.9f,  0.15f }, 0xFF28E6FF },
				{ "Blue",    { 0.2f,  0.4f,  1.0f  }, 0xFFFF6E32 },
				{ "Magenta", { 1.0f,  0.25f, 0.95f }, 0xFFF046FF },
				{ "Cyan",    { 0.15f, 0.95f, 1.0f  }, 0xFFFFF028 },
				{ "White",   { 1.0f,  1.0f,  1.0f  }, 0xFFFFFFFF },
			};
			constexpr int LIGHT_COLOR_COUNT = static_cast<int>(sizeof(LIGHT_COLOR_INFOS) / sizeof(LIGHT_COLOR_INFOS[0]));
			static_assert(LIGHT_COLOR_COUNT == static_cast<int>(LightColor::White) + 1, "LightColor と表の数が合わない");

			/** プロパティ欄で選べる色（None は選ばせない） */
			constexpr LightColor SELECTABLE_COLORS[] = {
				LightColor::White, LightColor::Red, LightColor::Green, LightColor::Blue,
				LightColor::Yellow, LightColor::Cyan, LightColor::Magenta,
			};


			const LightColorInfo& GetInfo(const LightColor color)
			{
				const int index = static_cast<int>(color);
				return LIGHT_COLOR_INFOS[(index >= 0 && index < LIGHT_COLOR_COUNT) ? index : 0];
			}
		}


		const char* GetLightColorName(const LightColor color)
		{
			return GetInfo(color).name;
		}


		bool FindLightColorByName(const std::string& name, LightColor& outColor)
		{
			for (int i = 0; i < LIGHT_COLOR_COUNT; ++i)
			{
				if (name == LIGHT_COLOR_INFOS[i].name)
				{
					outColor = static_cast<LightColor>(i);
					return true;
				}
			}
			return false;
		}


		void GetLightColorRGB(const LightColor color, float outRGB[3])
		{
			const LightColorInfo& info = GetInfo(color);
			outRGB[0] = info.rgb[0];
			outRGB[1] = info.rgb[1];
			outRGB[2] = info.rgb[2];
		}


		uint32_t GetLightColorEditorColor(const LightColor color)
		{
			return GetInfo(color).editorColor;
		}


		bool EditLightColor(const char* label, LightColor& color)
		{
			bool isChanged = false;
			if (ImGui::BeginCombo(label, GetLightColorName(color)))
			{
				for (const LightColor candidate : SELECTABLE_COLORS)
				{
					if (ImGui::Selectable(GetLightColorName(candidate), candidate == color))
					{
						isChanged = candidate != color;
						color = candidate;
					}
				}
				ImGui::EndCombo();
			}
			return isChanged;
		}


		void SerializeLightColor(GimmickArchive& archive, const char* key, LightColor& color)
		{
			// JSON には名前で書く（"Red" など）。手で書き換えるときにわかりやすいように
			std::string name = GetLightColorName(color);
			archive.Field(key, name);
			if (archive.IsReading())
			{
				// 知らない名前なら今の色のまま
				FindLightColorByName(name, color);
			}
		}
	}
}
