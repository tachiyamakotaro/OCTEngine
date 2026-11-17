#pragma once


/**
 * @file GoalParameter.h
 * @brief ゴール（遺跡の門）とクリアの演出の調整値（GoalEditor で変えられる値をまとめる）
 */
namespace app
{
	namespace stage
	{
		/**
		 * @brief 光る色1つぶん（色と、光る強さ）
		 * @details 色と光る強さはいつもセットで使うので、1つにまとめる（float をばらばらに持たない）
		 */
		struct GlowColor
		{
			/** 色 */
			float rgb[3] = { 1.0f, 1.0f, 1.0f };
			/** 光る強さ（1 より大きいとブルームで光があふれる） */
			float glow = 1.0f;

			/** シェーダーに渡す形（rgb = 色, a = 光る強さ） */
			Vector4 ToShaderValue() const { return Vector4(rgb[0], rgb[1], rgb[2], glow); }
			/** 2つの色の間（rate = 0 で from、1 で to） */
			static GlowColor Lerp(const GlowColor& from, const GlowColor& to, const float rate)
			{
				GlowColor result;
				for (int i = 0; i < 3; ++i)
				{
					result.rgb[i] = from.rgb[i] + (to.rgb[i] - from.rgb[i]) * rate;
				}
				result.glow = from.glow + (to.glow - from.glow) * rate;
				return result;
			}
		};


		/**
		 * @brief クリアの演出の時間（扉が開く → 少し待つ → 丸が閉じる → 真っ暗で少し待つ → 次のステージへ）
		 */
		struct ClearTiming
		{
			/** 扉が開ききるまでの時間（秒） */
			float doorOpenTime = 0.9f;
			/** 扉が開いてから、丸が閉じ始めるまでの時間（秒） */
			float waitAfterOpen = 0.35f;
			/** 丸が閉じきるまでの時間（秒） */
			float irisCloseTime = 1.1f;
			/** 真っ暗になってから、次のステージへ移り始めるまでの時間（秒） */
			float blackHoldTime = 0.25f;

			/** 丸が閉じ始める時間（クリアしてから） */
			float GetIrisStartTime() const { return doorOpenTime + waitAfterOpen; }
			/** 演出が全部終わる時間（クリアしてから） */
			float GetTotalTime() const { return GetIrisStartTime() + irisCloseTime + blackHoldTime; }
		};


		/**
		 * @brief 門の見た目（扉の開き方・扉の奥の光・松明の炎）
		 */
		struct GateStyle
		{
			/** 扉が開ききったときの角度（度。扉は奥へ開く） */
			float doorOpenAngleDeg = 80.0f;
			/** 確認用：クリアしていなくても、全部の扉をこの割合だけ開く（0 なら閉じたまま） */
			float previewOpenRate = 0.0f;

			/** 扉の奥の光（閉じているとき。合わせ目のすき間から見える） */
			GlowColor lightClosed = { { 1.0f, 0.78f, 0.45f }, 1.4f };
			/** 扉の奥の光（開ききったとき。真ん中ほど lightCore に近づく） */
			GlowColor lightOpen = { { 1.0f, 0.82f, 0.55f }, 1.8f };
			/** 扉の奥の光の真ん中（開ききったとき） */
			GlowColor lightCore = { { 1.0f, 0.97f, 0.88f }, 2.6f };

			/** 松明の炎を出すか */
			bool isTorchLit = true;
			/** 炎の外側（先のほう） */
			GlowColor flameOuter = { { 1.0f, 0.42f, 0.08f }, 2.0f };
			/** 炎の内側（根元・真ん中） */
			GlowColor flameInner = { { 1.0f, 0.88f, 0.5f }, 3.0f };
			/** 炎の幅と高さ */
			float flameWidth = 10.0f;
			float flameHeight = 24.0f;
			/** 炎のゆらめきの速さと大きさ（高さに対する割合） */
			float flickerSpeed = 9.0f;
			float flickerAmount = 0.12f;
		};
	}
}
