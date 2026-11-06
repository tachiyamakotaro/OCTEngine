#pragma once


/**
 * @file GimmickMarkParameter.h
 * @brief ギミックの印（電球マーク・歯車・レール・チェックポイントの旗）の調整値（GimmickMarkEditor で変えられる値をまとめる）
 */
namespace app
{
	namespace stage
	{
		/**
		 * @brief 印の色1つぶん（色と、自分で光る強さ）
		 * @details 色と光る強さはいつもセットで使うので、1つにまとめる（float をばらばらに持たない）
		 */
		struct MarkColor
		{
			/** 色 */
			float rgb[3] = { 1.0f, 1.0f, 1.0f };
			/** 自分で光る強さ（0 なら光らない。1 より大きいとブルームで光る） */
			float glow = 0.0f;

			/** シェーダーに渡す形（rgb = 色, a = 光る強さ） */
			Vector4 ToShaderValue() const { return Vector4(rgb[0], rgb[1], rgb[2], glow); }
		};


		/**
		 * @brief 受光器の電球マークの調整値
		 */
		struct BulbStyle
		{
			/** 描くか */
			bool isVisible = true;
			/** 大きさ（受光器の幅と高さの小さいほうに対する割合） */
			float scale = 0.95f;
			/** 光が当たっていないとき（白っぽくして、少しだけ光らせる。黄色の上でも暗い所でも見えるように） */
			MarkColor off = { { 0.92f, 0.92f, 0.86f }, 0.35f };
			/** 光が当たっているとき（強く光る） */
			MarkColor lit = { { 1.0f, 0.97f, 0.82f }, 1.6f };
			/** 縁取り（黄色の上でも形がわかるように、こげ茶） */
			MarkColor outline = { { 0.22f, 0.14f, 0.04f }, 0.0f };
		};


		/**
		 * @brief 回せる反射鏡の歯車の調整値
		 */
		struct GearStyle
		{
			/** 描くか */
			bool isVisible = true;
			/** 直径 */
			float size = 44.0f;
			/** 歯の数（45度回して見た目が変わるよう、8 の約数・倍数でない数にする） */
			float teeth = 10.0f;
			/** 板の厚さ */
			float thickness = 4.0f;
			/** 本体（真ちゅう。暗い所でも少し見えるように、少しだけ光る） */
			MarkColor body = { { 0.78f, 0.58f, 0.24f }, 0.12f };
			/** 縁・軸 */
			MarkColor dark = { { 0.18f, 0.12f, 0.06f }, 0.0f };
		};


		/**
		 * @brief 動かせる反射鏡のレールの調整値
		 */
		struct RailStyle
		{
			/** 描くか */
			bool isVisible = true;
			/** 高さ（太さ） */
			float height = 14.0f;
			/** 奥行き */
			float depth = 16.0f;
			/** 動かせる範囲より、両端をどれだけ長くするか（0 なら範囲とぴったり同じ長さ） */
			float extend = 0.0f;
			/** 枕木（レールの中の区切り）の間隔 */
			float tieSpacing = 24.0f;
			/** 本体（鉄） */
			MarkColor body = { { 0.62f, 0.64f, 0.68f }, 0.1f };
			/** みぞ・縁 */
			MarkColor dark = { { 0.12f, 0.12f, 0.14f }, 0.0f };
			/** 両端の止め金の幅 */
			float stopperWidth = 10.0f;
			/** 両端の止め金の高さ */
			float stopperHeight = 28.0f;
			/** 両端の止め金 */
			MarkColor stopper = { { 0.35f, 0.33f, 0.32f }, 0.1f };
		};


		/**
		 * @brief 印を置く奥行き（ギミックの本体の大きさに合わせる）
		 */
		struct MarkPlacement
		{
			/** 受光器の奥行き（GimmickRenderer の receiverDepth と同じにする。電球はこの手前の面に貼る） */
			float receiverDepth = 60.0f;
			/** 反射鏡の奥行き（GimmickRenderer の lightMirrorDepth と同じにする。歯車はこの手前、レールは真ん中（光のすぐ奥）に置く） */
			float lightMirrorDepth = 80.0f;
			/** 細い光の太さ（LightBeamRenderer の thinBeamWidth と同じにする。レールはこの光より奥に置いて、光を隠さない） */
			float lightBeamWidth = 8.0f;
			/** 面と印の板のすき間（面にぴったり重なるとちらつくので少し離す） */
			float surfaceGap = 1.0f;
		};


		/**
		 * @brief 旗のはためき方（振れ幅と速さはいつもセットで使うので1つにまとめる）
		 */
		struct FlagWave
		{
			/** 振れ幅（旗の高さの半分に対する割合。先にいくほど大きく揺れる） */
			float amplitude = 0.1f;
			/** 速さ（1秒に進む波の角度。ラジアン） */
			float speed = 3.0f;
		};


		/**
		 * @brief チェックポイントの旗の調整値
		 * @details 旗はチェックポイントの範囲の下端の真ん中に立つ（台 → ポール → 先の玉 → 三角の旗）。
		 *          通る前は白い旗がゆっくりはためき、通ると赤に変わって一瞬大きくなり、元気にはためく。
		 */
		struct FlagStyle
		{
			/** 描くか */
			bool isVisible = true;
			/** 奥行きの位置（+ が奥。プレイヤー（Z = 0）より奥に立てて、体を隠さないようにする） */
			float z = 70.0f;

			// ---- 台 ----
			/** 台の幅（奥行きも同じ） */
			float baseWidth = 44.0f;
			/** 台の高さ */
			float baseHeight = 16.0f;
			/** 台の色（石） */
			MarkColor base = { { 0.45f, 0.43f, 0.40f }, 0.0f };

			// ---- ポール ----
			/** ポールの太さ */
			float poleWidth = 7.0f;
			/** ポールの高さ（台の上から。プレイヤー（120）より高くして、遠くから見えるようにする） */
			float poleHeight = 210.0f;
			/** ポールの色（銀色。暗い所でも少し見えるよう、少しだけ光る） */
			MarkColor pole = { { 0.80f, 0.80f, 0.84f }, 0.08f };

			// ---- 先の玉 ----
			/** 玉の直径 */
			float knobSize = 16.0f;
			/** 玉の色（金色） */
			MarkColor knob = { { 0.96f, 0.78f, 0.26f }, 0.35f };

			// ---- 旗 ----
			/** 旗の幅（ポールから先までの長さ） */
			float clothWidth = 86.0f;
			/** 旗の高さ（ポール側の長さ） */
			float clothHeight = 56.0f;
			/** 旗の中にある波の数 */
			float waveCount = 1.2f;
			/** 通る前の旗（白） */
			MarkColor clothOff = { { 0.95f, 0.95f, 0.92f }, 0.25f };
			/** 通ったあとの旗（赤） */
			MarkColor clothOn = { { 0.92f, 0.12f, 0.10f }, 0.45f };
			/** 旗の縁取り */
			MarkColor outline = { { 0.16f, 0.12f, 0.10f }, 0.0f };
			/** 通る前のはためき方（ゆっくり小さく） */
			FlagWave waveOff = { 0.06f, 2.0f };
			/** 通ったあとのはためき方（速く大きく） */
			FlagWave waveOn = { 0.16f, 6.0f };

			// ---- 白 → 赤に変わるとき ----
			/** 白から赤に変わりきるまでの時間（秒） */
			float changeTime = 0.35f;
			/** 変わる途中で旗が大きくなる量（0.35 なら一番大きいときに 1.35 倍） */
			float popScale = 0.35f;
		};


		/**
		 * @brief ギミックの印の調整値
		 * @details 値を足すときは、このファイル・GimmickMarkEditor・仕様書の「調整値」の表の3か所を直す
		 */
		struct GimmickMarkParameter
		{
			/** 受光器の電球マーク */
			BulbStyle bulb;
			/** 回せる反射鏡の歯車 */
			GearStyle gear;
			/** 動かせる反射鏡のレール */
			RailStyle rail;
			/** チェックポイントの旗 */
			FlagStyle flag;
			/** 印を置く奥行き */
			MarkPlacement placement;
		};
	}
}
