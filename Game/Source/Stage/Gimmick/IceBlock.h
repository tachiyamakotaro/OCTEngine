/**
 * @file IceBlock.h
 * @brief 氷（光が当たった所から少しずつ穴が開いていく）
 */
#pragma once
#include <cstdint>
#include <vector>
#include "RectGimmick.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 氷の種類（ゲームの動きは同じ。見た目と、ステージの中での役割が違う）
		 */
		enum class IceKind
		{
			Melt,	//!< 溶かす氷：道をふさいでいる。光を当てて溶かすと先へ進める（白っぽい水色・ひび）
			Keep,	//!< 守る氷：足場になっている。溶けると渡れなくなるので、日陰に入れて守る（濃い青・白い縁）
		};


		/**
		 * @brief 氷
		 * @details 氷を cellSize の大きさの小さなマスに分けて持ち、マスごとに溶かす。
		 *          ・光の線分のうち、氷の範囲の中を通る部分（当たった点や、掘った穴の中を通る部分）のまわりのマスに熱がたまる。
		 *            光に近いマスほど熱が早くたまり、熱がいっぱいになったマスは溶けて消える。
		 *            → 光が当たった一点からくぼみができ、光の進む向きにトンネルのように掘れていく。貫通すると光は奥まで届く。
		 *          ・熱が伝わる半径は、光が当たっている時間が長いほど広がる（heatRadius → maxHeatRadius）。
		 *            → 当て続けると、穴のまわりもじわじわ溶けて穴が太くなる（入口ほど長く温まるので広くなる）。
		 *          ・プレイヤーが通れる太さの穴（passWidth × passHeight の箱が通り抜けられる穴）が開いたら、
		 *            穴から熱が伝わって、残りの氷もまわりへ流れるように溶けていく（flowTime 秒で1マスずつ広がる）。
		 *            縦長の氷（壁）は左右に、横長の氷（床・天井）は上下に通り抜けられるかを調べる。
		 *          ・光を外すと溶けるのは止まる。溶けた分は戻らない（リスタートで戻る）。
		 *          ・当たり判定も光を止める範囲も、残っているマスだけ。
		 */
		class IceBlock : public RectGimmick
		{
		public:
			/** 種類（見た目と役割） */
			IceKind kind = IceKind::Melt;
			/** 溶ける速さ：光をまっすぐ当てたとき、厚さ 100 を掘り抜くまでのおよその秒数 */
			float meltTime = 2.0f;
			/** マスの大きさ（小さいほど穴の形が細かくなるが、重くなる） */
			float cellSize = 10.0f;
			/** 熱が伝わる半径（光が当たり始めたとき。光からこの距離までのマスが温まる） */
			float heatRadius = 25.0f;
			/** 熱が伝わる半径が広がる速さ（光が当たっている1秒あたり） */
			float heatGrowSpeed = 30.0f;
			/** 熱が伝わる半径の上限 */
			float maxHeatRadius = 150.0f;
			/** 通れる穴が開いたあと、溶けるのがとなりのマスへ伝わるまでの秒数（小さいほど早く全部溶ける） */
			float flowTime = 0.03f;
			/** 通れる穴の幅（プレイヤーの体の幅＋少し） */
			float passWidth = 60.0f;
			/** 通れる穴の高さ（プレイヤーの体の高さ＋少し） */
			float passHeight = 130.0f;

			/** 【実行時】今フレーム光が当たっているか（溶けている最中か） */
			bool isLit = false;
			/** 【実行時】光が当たっていた時間の合計（熱が伝わる半径が広がる） */
			float litTime = 0.0f;
			/** 【実行時】通れる穴が開いて、残りの氷が流れるように溶けている最中か */
			bool isFlowing = false;
			/** 【実行時】このフレームに通れる穴が開いたか（音や演出のきっかけに使う） */
			bool isPassableThisFrame = false;
			/** 【実行時】このフレームに溶けきったか（湯気のエフェクトや音を出すきっかけに使う） */
			bool isMeltedThisFrame = false;


		public:
			const char* GetTypeName() const override { return "IceBlock"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<IceBlock>(*this); }

			/** 光の計算の結果を見て溶ける */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Light; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override;
			/** 残っているマス全体を囲む矩形（細かい当たり判定は QueryCollision） */
			CollisionKind GetCollision(math::Rect& outRect) const override;
			/** box と重なる、残っているマスの当たり判定を足す（となりのマスはまとめた矩形にする） */
			CollisionKind QueryCollision(const math::Rect& box, std::vector<math::Rect>* out) const override;

			/** 光が通る順にマスを調べ、最初の残っているマスで止める */
			bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const override;

			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 溶けきったか（残っているマスがない） */
			bool IsMelted() const;
			/** 溶けた割合（0=元の大きさ 〜 1=溶けきった。マスの数で数える） */
			float GetMeltAmount() const;
			/**
			 * @brief 残っているマスを、なるべく大きな矩形にまとめて out に入れる（描画・当たり判定用）
			 * @details 横に並んだマスを1本にまとめ、すぐ上の行も同じ幅ならさらに縦につなげる
			 */
			void CollectSolidRects(std::vector<math::Rect>& out) const;
			/** マスの数（横・縦） */
			int GetColumnCount() const { return m_columns; }
			int GetRowCount() const { return m_rows; }
			/** マスが残っているか（範囲外なら false） */
			bool IsCellSolid(int column, int row) const;
			/** 縦長の氷か（縦長なら左右に、横長なら上下に通り抜けられるかを調べる） */
			bool IsVertical() const { return rect.Height() >= rect.Width(); }
			/** プレイヤーが通れる穴が開いているか（毎回調べるので少し重い） */
			bool HasPassage() const;


		public:
			/** x, y は左下 */
			IceBlock(const char* name, float x, float y, float width, float height, IceKind initialKind);


		private:
			/** 今の大きさとマスの大きさに合わせて、マスを作り直す（大きさが変わっていなければ何もしない） */
			void EnsureGrid();
			/** マスの数が今の大きさと合っているか */
			bool IsGridValid() const;
			/** マスの数を計算する */
			void CalcGridSize(int& outColumns, int& outRows) const;
			/** マスの幅・高さ（ワールド） */
			float GetCellWidth() const { return m_columns > 0 ? rect.Width() / static_cast<float>(m_columns) : rect.Width(); }
			float GetCellHeight() const { return m_rows > 0 ? rect.Height() / static_cast<float>(m_rows) : rect.Height(); }
			/** マスの番号 */
			int CellIndex(int column, int row) const { return row * m_columns + column; }
			/** マスの矩形（ワールド） */
			math::Rect GetCellRect(int column, int row) const;
			/** 光の線分ごとに、まわりのマスに熱をためる。熱がたまったら true */
			bool HeatFromLight(const GimmickUpdateContext& context);
			/** 溶けたマスのとなりのマスに熱をためる（通れる穴が開いたあと） */
			void HeatFromHole(float deltaTime);
			/** 熱がいっぱいになったマスを溶かす。溶けたマスがあれば true */
			bool MeltHotCells();


		private:
			/** マスの数（横・縦） */
			int m_columns = 0;
			int m_rows = 0;
			/** マスが残っているか（1=残っている） */
			std::vector<std::uint8_t> m_solid;
			/** マスにたまった熱（1 で溶ける） */
			std::vector<float> m_heat;
			/** 残っているマスの数 */
			int m_solidCount = 0;
		};


		/** @brief 種類 → JSON に書く文字（"Melt" / "Keep"） */
		const char* IceKindToString(IceKind kind);
		/** @brief JSON の文字 → 種類（知らない文字なら false で、out は変えない） */
		bool IceKindFromString(const std::string& text, IceKind& out);
	}
}
