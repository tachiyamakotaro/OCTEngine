#pragma once
#include "Source/Core/Math/Rect.h"
#include <cstdint>
#include <vector>


/**
 * @file BackgroundLayout.h
 * @brief 背景の回廊の区画と松明を、ステージの範囲に合わせて並べる（どこに何を置くかだけ。描くのは BackgroundRenderer）
 */
namespace app
{
	namespace stage
	{
		/**
		 * @brief 背景のモデルの形の値（Tools/gen_background_models.py と同じ値。モデルを作り直して形を変えたら、ここも合わせる）
		 * @details モデルは実寸。層の大きさ（BackgroundLayerStyle::scale）をかけて置く
		 */
		namespace background_shape
		{
			/** 区画の幅（柱から次の柱まで） */
			constexpr float BAY_WIDTH = 640.0f;
			/** 区画の高さ（床から上の階の床まで） */
			constexpr float TIER_HEIGHT = 900.0f;
			/** 松明の原点（炎の根元）から、柱の手前の面までの距離 */
			constexpr float TORCH_DEPTH = 36.0f;
		}


		/** 区画の種類 */
		enum class BackgroundBayKind
		{
			Whole,	//!< 柱・アーチ・上の階の床がそろっている
			Broken,	//!< アーチが崩れている（柱と上の階の床は残っている）
		};


		/**
		 * @brief 回廊の区画1つ
		 */
		struct BackgroundBay
		{
			/** 左の柱の足元（層の中の位置。描くときにカメラについてくる分を足す） */
			math::Vec2 foot;
			/** 種類 */
			BackgroundBayKind kind = BackgroundBayKind::Whole;
		};


		/**
		 * @brief 柱に付けた松明1つ
		 */
		struct BackgroundTorch
		{
			/** 炎の根元（層の中の位置） */
			math::Vec2 root;
			/** ゆらめきをずらす角度（松明ごとに違う） */
			float phase = 0.0f;
		};


		/**
		 * @brief 並べ方を決める値（これと範囲が同じなら、並べ直さなくてよい）
		 */
		struct BackgroundLayoutSetting
		{
			/** モデルの大きさ */
			float scale = 1.0f;
			/** カメラについてくる割合（大きいほど、カメラが動いたときに見える所が広いので、広く並べる） */
			float parallax = 0.0f;
			/** 何本おきの柱に松明を付けるか（0 なら付けない） */
			int torchInterval = 2;
			/** 区画の床からの炎の根元の高さ（大きさをかける前） */
			float torchHeight = 300.0f;
			/** アーチが崩れた区画の割合（0〜1） */
			float brokenRate = 0.0f;
			/** 崩れた区画を選ぶ乱数の種 */
			std::uint32_t seed = 1;

			bool operator==(const BackgroundLayoutSetting& o) const
			{
				return scale == o.scale && parallax == o.parallax && torchInterval == o.torchInterval
					&& torchHeight == o.torchHeight && brokenRate == o.brokenRate && seed == o.seed;
			}
			bool operator!=(const BackgroundLayoutSetting& o) const { return !(*this == o); }
		};


		/**
		 * @brief 背景の層1つぶんの並び（区画と松明）
		 * @details ・区画はステージの範囲の左下から、幅 BAY_WIDTH・高さ TIER_HEIGHT（どちらも大きさをかける）のマス目に並べる
		 *          ・カメラについてくる層は、カメラが端にいるときに層のもっと外側が見えるので、そのぶん広く並べる
		 *            （カメラがステージの範囲の中にいれば、ステージの範囲の中に見える所はどこも埋まる）
		 *          ・崩れた区画は、マス目の位置から作った乱数で選ぶ（同じ範囲・同じ種なら、毎回同じ場所が崩れる）
		 */
		class BackgroundLayerLayout
		{
		public:
			BackgroundLayerLayout() = default;

			/**
			 * @brief ステージの範囲に合わせて並べる
			 * @param stageBounds ステージの範囲（全部の地形ブロックを囲む矩形）
			 */
			static BackgroundLayerLayout Build(const math::Rect& stageBounds, const BackgroundLayoutSetting& setting);

			/** 区画（下の段から、左から順） */
			const std::vector<BackgroundBay>& GetBays() const { return m_bays; }
			/** 松明 */
			const std::vector<BackgroundTorch>& GetTorches() const { return m_torches; }
			/** 並べた範囲（層の中の位置） */
			const math::Rect& GetArea() const { return m_area; }

		private:
			/** 区画 */
			std::vector<BackgroundBay> m_bays;
			/** 松明 */
			std::vector<BackgroundTorch> m_torches;
			/** 並べた範囲 */
			math::Rect m_area;
		};
	}
}
