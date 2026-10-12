/**
 * @file MirrorGlass.h
 * @brief 本物の世界と鏡の中の世界の境目に置く、巨大な半透明のガラス（見た目だけ）
 * @details 奥の鏡の面（BackMirror::planeZ）に、鏡の枠（BackMirror::frame）と同じ大きさの板を置く。
 *          手前に本物のプレイヤー、奥に鏡に映ったプレイヤーがいて、その間をスタートからゴールまで仕切る。
 *          鏡の中の世界がこの板ごしに少し色付いて見えるので、「鏡の向こう側」らしくなる。
 *
 *          ・当たり判定も光の計算にも関係しない（見た目だけ）
 *          ・色・光の筋・パネルの継ぎ目・縁の光はシェーダー（Assets/shader/mirrorGlass.fx）で付ける。
 *            値は Config で変えられる（デバッグウィンドウの Display → Mirror Glass）
 *          ・半透明なので、ほかのモデルを全部 Draw したあとで Draw すること
 *            （エンジンは登録した順に描くため。先に描くと後ろのモデルが透けて見えない）
 *
 *          エンジンの ModelRender は model.fx 固定なので、Model を直接持ってシェーダーと半透明の設定を渡している。
 */
#pragma once


namespace app
{
	namespace stage
	{
		struct BackMirror;
	}


	namespace render
	{
		/**
		 * @brief 鏡の境目の巨大な半透明ガラス
		 */
		class MirrorGlass
		{
		public:
			/** 見た目の設定 */
			struct Config
			{
				/** 表示するか */
				bool isVisible = true;
				/** 鏡の面からどれだけ手前に置くか（床の奥の端と重なってちらつかないように少しずらす） */
				float zOffset = 1.0f;
				/** 板の厚さ */
				float thickness = 2.0f;
				/** 地の色（rgb）と不透明度（a） */
				float tint[4] = { 0.55f, 0.78f, 1.0f, 0.16f };
				/** 縁の光の色（1 より大きいとブルームで光る） */
				float edgeColor[3] = { 1.0f, 1.25f, 1.7f };
				/** 縁の光の幅（ワールド単位） */
				float edgeWidth = 60.0f;
				/** 光の筋の色 */
				float sheenColor[3] = { 0.85f, 0.95f, 1.0f };
				/** 光の筋の不透明度 */
				float sheenOpacity = 0.14f;
				/** 光の筋の間隔（ワールド単位） */
				float sheenSpacing = 1800.0f;
				/** 光の筋が流れる速さ（1秒あたり、間隔に対する割合） */
				float sheenSpeed = 0.04f;
				/** 斜めから見たときに不透明になる強さ */
				float fresnel = 0.6f;
				/** 上ほど明るくする量 */
				float topBrightness = 0.6f;
				/** パネル（大きな鏡1枚）の大きさ。0 で継ぎ目なし */
				float panelSize = 700.0f;
				/** 継ぎ目の太さ（ワールド単位） */
				float seamWidth = 3.0f;
				/** 継ぎ目の不透明度 */
				float seamOpacity = 0.18f;
				/** 全体の不透明度の倍率（0 で見えない） */
				float opacity = 1.0f;
			};


		public:
			/** @brief モデルとシェーダーを読み込む */
			void Init();
			/**
			 * @brief 奥の鏡に合わせて位置・大きさを決め、シェーダーに渡す値を更新する（毎フレーム）
			 * @param backMirror 奥の鏡（この枠と面に合わせる）
			 * @param deltaTime 経過時間（光の筋を流すのに使う）
			 */
			void Update(const stage::BackMirror& backMirror, float deltaTime);
			/** @brief 描く（ほかのモデルを全部 Draw したあとに呼ぶ） */
			void Draw(RenderContext& rc);

			inline Config& GetConfig() { return m_config; }
			inline const Config& GetConfig() const { return m_config; }
			/** @brief 今のフレームに描くか（奥の鏡が OFF か、isVisible が false なら描かない） */
			inline bool IsDrawn() const { return m_isInited && m_isActive; }


		public:
			MirrorGlass();


		private:
			/**
			 * @brief シェーダーに渡す値（mirrorGlass.fx の MirrorGlassCb と同じ並び。全部 float4 にしている）
			 */
			struct Constants
			{
				float tintColor[4];		// rgb = 地の色, a = 地の不透明度
				float edgeColor[4];		// rgb = 縁の光の色, a = 縁の光の幅
				float sheenColor[4];	// rgb = 光の筋の色, a = 光の筋の不透明度
				float rect[4];			// 板の範囲 (minX, minY, maxX, maxY)
				float eyePosTime[4];	// xyz = カメラの位置, w = 時間
				float param[4];			// x = フレネル, y = 上ほど明るく, z = 光の筋の間隔, w = 光の筋の速さ
				float panel[4];			// x = パネルの大きさ, y = 継ぎ目の太さ, z = 継ぎ目の不透明度, w = 全体の不透明度
			};


		private:
			/** モデル（ModelRender ではなく、シェーダーを選べる Model を直接使う） */
			Model m_model;
			/** シェーダーに渡す値 */
			Constants m_constants;
			/** 見た目の設定 */
			Config m_config;
			/** 経過時間 */
			float m_time;
			/** 読み込んだか */
			bool m_isInited;
			/** 今のフレームに描くか */
			bool m_isActive;
		};
	}
}
