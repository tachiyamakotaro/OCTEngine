/**
 * @file LightColor.h
 * @brief 光の色（赤・緑・青の3つの組み合わせ）
 * @details 光の色は「赤・緑・青のどれが入っているか」の3ビットで表す。
 *            赤 = 1, 緑 = 2, 青 = 4
 *            黄 = 赤 + 緑, 水色 = 緑 + 青, 赤紫 = 赤 + 青, 白 = 赤 + 緑 + 青
 *          こうしておくと、現実の光と同じ計算が1行でできる。
 *            ・光を重ねる（混色）   … ビットの OR（赤い光 + 緑の光 = 黄色の光）
 *            ・色ガラスを通す       … ビットの AND（白い光が赤いガラスを通ると赤い光。青い光は赤いガラスを通れない）
 */
#pragma once
#include <cstdint>
#include <string>


namespace app
{
	namespace stage
	{
		class GimmickArchive;


		/**
		 * @brief 光の色
		 */
		enum class LightColor : uint8_t
		{
			None = 0,		//!< 光なし（色ガラスで全部さえぎられた）
			Red = 1,		//!< 赤
			Green = 2,		//!< 緑
			Yellow = 3,		//!< 黄（赤 + 緑）
			Blue = 4,		//!< 青
			Magenta = 5,	//!< 赤紫（赤 + 青）
			Cyan = 6,		//!< 水色（緑 + 青）
			White = 7,		//!< 白（赤 + 緑 + 青）
		};


		/** @brief 2つの光を重ねた色（光の混色：赤 + 緑 = 黄） */
		inline LightColor MixLightColor(const LightColor a, const LightColor b)
		{
			return static_cast<LightColor>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
		}
		/** @brief 光が色ガラスを通ったあとの色（ガラスと同じ色の成分だけ残る：白 → 赤いガラス → 赤） */
		inline LightColor FilterLightColor(const LightColor light, const LightColor glass)
		{
			return static_cast<LightColor>(static_cast<uint8_t>(light) & static_cast<uint8_t>(glass));
		}


		/** @brief 色の名前（"Red" など。JSON とエディタの表示に使う） */
		const char* GetLightColorName(LightColor color);
		/**
		 * @brief 名前から色を探す
		 * @return 見つかったら true（見つからなければ outColor は変えない）
		 */
		bool FindLightColorByName(const std::string& name, LightColor& outColor);
		/**
		 * @brief 描くときの色（0〜1 の rgb）
		 * @details 白は描画側の今までの光の色を使うので、ここでは (1, 1, 1) を返す
		 */
		void GetLightColorRGB(LightColor color, float outRGB[3]);
		/** @brief エディタの見取り図で使う色（IM_COL32 と同じ形の 32 ビット） */
		uint32_t GetLightColorEditorColor(LightColor color);

		/** @brief プロパティ欄の色の選択（ImGui のコンボボックス）。変更されたら true */
		bool EditLightColor(const char* label, LightColor& color);
		/** @brief JSON への書き出し・読み込み（"color": "Red" の形。知らない名前なら読まずに今の値のまま） */
		void SerializeLightColor(GimmickArchive& archive, const char* key, LightColor& color);
	}
}
