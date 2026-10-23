/**
 * @file GimmickBase.h
 * @brief ステージに置く物（ギミック）の基底クラス
 * @details ステージに置く物は、地形ブロックもふくめて全部このクラスを継承する。
 *          Stage・光の計算・エディタは「GimmickBase の仮想関数」だけを呼ぶので、
 *          新しいギミックを足すときは、このクラスを継承したクラスを1つ作って、
 *          GimmickRegistry.cpp に1行足すだけでよい（Stage やエディタは書き換えなくてよい）。
 *
 *          仮想関数はグループごとに分かれていて、必要なものだけ override すればよい。
 *          ・ゲーム中   … 更新・リセット・当たり判定
 *          ・光         … 光を出す・光に当たる
 *          ・信号       … 受光器やスイッチのように「名前で ON/OFF を出す」物と、それを見る物
 *          ・エディタ   … 位置・見取り図への描画・プロパティ
 *          ・ファイル   … JSON への書き出しと読み込み（Serialize）
 *          「= 0」の付いた関数だけは、必ず override する。
 */
#pragma once
#include "Source/Core/Math/Rect.h"
#include "Source/Stage/LightColor.h"
#include <memory>
#include <string>
#include <vector>


namespace app
{
	namespace stage
	{
		class GimmickManager;
		struct LightTraceResult;
		class EditorCanvas;
		class GimmickInspector;
		class GimmickArchive;


		/**
		 * @brief 更新の順番
		 * @details ギミックどうしは「スイッチが押される → 光が届く → 扉が開く → 霧が出る → 足場が見える」のように
		 *          ほかのギミックの結果を使うので、決まった順番で更新する。Stage::Update が上から順に呼ぶ。
		 */
		enum class UpdatePhase
		{
			Input,	//!< キャラクターに踏まれる・触れられる（光の計算の前）
			Light,	//!< 光の計算の結果を受け取る
			Signal,	//!< 信号（受光器・スイッチ）を見て動く
			Late,	//!< ほかのギミックの動いた結果を見て動く
		};


		/**
		 * @brief 当たり判定の種類
		 */
		enum class CollisionKind
		{
			None,		//!< 当たり判定なし
			Terrain,	//!< 地形（ブロック・閉じた扉など。反射鏡を動かすときにもぶつかる）
			Platform,	//!< 乗れるが地形ではない（光の足場。反射鏡はすり抜ける）
		};


		/**
		 * @brief 光が当たったときの反応
		 */
		enum class LightResponse
		{
			Stop,			//!< 光が止まる（ブロック・扉・鏡の裏）
			Reflect,		//!< 反射する（反射鏡）
			Absorb,			//!< 光が止まり、当たったギミックが点灯する（受光器）
			PassThrough,	//!< 光は通り抜け、通ったギミックが点灯する（光の足場）
			Filter,			//!< 光は通り抜けるが、そこから先の光の色が変わる（色ガラス。LightHit::filterColor と同じ色の成分だけ残る）
		};


		/**
		 * @brief 光が当たった結果
		 */
		struct LightHit
		{
			/** 光の始点から当たった点までの距離 */
			float distance = 0.0f;
			/** 反応 */
			LightResponse response = LightResponse::Stop;
			/** 反射するときの面の向き（光が来た側を向いた法線） */
			math::Vec2 normal;
			/** Filter のとき、通ったあとに残る色（白 = 何も変わらない） */
			LightColor filterColor = LightColor::White;
		};


		/**
		 * @brief ギミックが出す光1本
		 * @details 光源は GetEmittedRay で1本だけ返せばよい。日光のように何本も出す物は CollectEmittedRays で並べる。
		 */
		struct EmittedRay
		{
			/** 光が出る位置 */
			math::Vec2 origin;
			/** 光の向き（長さ1） */
			math::Vec2 direction;
			/** 光の太さ（0 ならふつうの細い光） */
			float width = 0.0f;
		};


		/**
		 * @brief 更新のときに渡す情報
		 */
		struct GimmickUpdateContext
		{
			/** 経過時間 */
			float deltaTime;
			/** スイッチを踏める体（キャラクターの当たり判定の箱） */
			const std::vector<math::Rect>& bodies;
			/** ステージの全ギミック（信号を調べたり、ほかのギミックを探したりする） */
			const GimmickManager& gimmicks;
			/** 今フレームの光の計算結果 */
			const LightTraceResult& light;
		};


		/**
		 * @brief エディタの見取り図で描く順番（後ろから順に描き、手前から順にクリックを調べる）
		 */
		enum class EditorLayer
		{
			Back,	//!< 地形ブロック
			Middle,	//!< ふつうのギミック
			Front,	//!< 光より手前に描く物（反射鏡・光源）
		};


		/**
		 * @brief どちらのファイルに保存するか
		 * @details ステージは「地形のファイル」と「ギミックのファイル」の2つに分けて保存する。
		 *          読み込みはどちらのファイルに何が書いてあってもよい（保存するときの振り分けだけに使う）。
		 */
		enum class StageFileKind
		{
			Object,		//!< 地形のファイル（StageXX_Object.json）
			Gimmick,	//!< ギミックのファイル（StageXX_Gimmick.json）
		};


		/**
		 * @brief ギミックの基底クラス
		 */
		class GimmickBase
		{
		public:
			/** 名前（エディタの表示と、信号のつながりに使う） */
			std::string name;


		public:
			// ---- 種類 ----
			/** @brief クラス名（"Door" など。エディタの一覧と C++ コードの出力に使う） */
			virtual const char* GetTypeName() const = 0;
			/** @brief 自分のコピーを作る（エディタの複製と Undo に使う） */
			virtual std::unique_ptr<GimmickBase> Clone() const = 0;


		public:
			// ---- ゲーム中 ----
			/** @brief 何番目に更新するか */
			virtual UpdatePhase GetUpdatePhase() const { return UpdatePhase::Signal; }
			/** @brief 毎フレームの更新 */
			virtual void Update(const GimmickUpdateContext& context) {}
			/** @brief 実行時の状態（扉の開き具合・点灯など）を最初に戻す */
			virtual void ResetRuntimeState() {}
			/**
			 * @brief 当たり判定
			 * @param outRect 当たり判定の範囲（判定があるときだけ書き込む）
			 */
			virtual CollisionKind GetCollision(math::Rect& outRect) const { return CollisionKind::None; }
			/**
			 * @brief box と重なる当たり判定を out に足す
			 * @details ふつうは GetCollision の矩形1つ。当たり判定が1つの矩形で表せない物（偽物の壁）だけ override する
			 * @param out 足す先（nullptr なら、重なるかどうかだけ調べる）
			 * @return 重なった当たり判定の種類（重ならなければ None）
			 */
			virtual CollisionKind QueryCollision(const math::Rect& box, std::vector<math::Rect>* out) const
			{
				math::Rect collision;
				const CollisionKind kind = GetCollision(collision);
				if (kind == CollisionKind::None || !box.Overlaps(collision))
				{
					return CollisionKind::None;
				}
				if (out != nullptr)
				{
					out->push_back(collision);
				}
				return kind;
			}
			/**
			 * @brief このフレームで、足元の床がどれだけ動いたか（動く床だけ override する）
			 * @details キャラクターは、乗っている床が動いた分だけ一緒に動く（CharacterController::Move）。
			 * @param groundProbe キャラクターの足元のすぐ下の薄い箱（前のフレームの位置）
			 * @param outDelta 床が動いた量
			 * @return この床の上に立っていたら true
			 */
			virtual bool GetCarryDelta(const math::Rect& groundProbe, math::Vec2& outDelta) const { return false; }


		public:
			// ---- 光 ----
			/**
			 * @brief 光を出しているか（光源だけ override する）
			 * @param outOrigin 光が出る位置
			 * @param outDirection 光の向き（長さ1）
			 */
			virtual bool GetEmittedRay(math::Vec2& outOrigin, math::Vec2& outDirection) const { return false; }
			/** @brief 出している光の太さ（0 ならふつうの細い光。光源だけ override する） */
			virtual float GetEmittedBeamWidth() const { return 0.0f; }
			/** @brief 出している光の色（ふつうは白。光源だけ override する） */
			virtual LightColor GetEmittedColor() const { return LightColor::White; }
			/**
			 * @brief 出している光を全部 out に足す（光の計算はこれを呼ぶ）
			 * @details ふつうは GetEmittedRay の1本。日光のように何本も出す物だけ override する
			 */
			virtual void CollectEmittedRays(std::vector<EmittedRay>& out) const
			{
				EmittedRay ray;
				if (GetEmittedRay(ray.origin, ray.direction))
				{
					ray.width = GetEmittedBeamWidth();
					out.push_back(ray);
				}
			}
			/**
			 * @brief 光が当たるか調べる（光に反応する物だけ override する）
			 * @param origin 光の始点
			 * @param direction 光の向き（長さ1）
			 * @param outHit 当たった結果
			 * @return 当たったら true
			 */
			virtual bool IntersectLight(const math::Vec2& origin, const math::Vec2& direction, LightHit& outHit) const { return false; }


		public:
			// ---- 信号 ----
			/** @brief 信号を出す物か（受光器・スイッチ） */
			virtual bool IsSignalSource() const { return false; }
			/** @brief 信号が ON か */
			virtual bool IsSignalOn() const { return false; }
			/** @brief 信号の名前が変わったので、つながりを付け替える（信号を見る物だけ override する） */
			virtual void RenameSignal(const std::string& oldName, const std::string& newName) {}
			/** @brief 信号が消えたので、つながりから外す（信号を見る物だけ override する） */
			virtual void RemoveSignal(const std::string& signalName) {}


		public:
			// ---- エディタ ----
			/** @brief ドラッグで動かすときの基準点 */
			virtual math::Vec2 GetAnchor() const = 0;
			/** @brief 基準点を動かす（大きさは変えない） */
			virtual void SetAnchor(const math::Vec2& anchor) = 0;
			/** @brief 全体を囲む矩形 */
			virtual math::Rect GetBounds() const = 0;
			/**
			 * @brief クリックした点に当たっているか
			 * @param point クリックした位置（ワールド）
			 * @param pickDistance 当たりとみなす距離（ワールド）
			 */
			virtual bool HitTest(const math::Vec2& point, float pickDistance) const;
			/** @brief 見取り図で描く順番 */
			virtual EditorLayer GetEditorLayer() const { return EditorLayer::Middle; }
			/** @brief 見取り図に描く */
			virtual void DrawEditor(const EditorCanvas& canvas, bool isSelected) const = 0;
			/** @brief プロパティ欄（ImGui）。名前の欄はエディタが描くので、ここでは描かない */
			virtual void DrawInspector(GimmickInspector& inspector) {}
			/**
			 * @brief 90度ずつ回す（エディタの回転ボタン・R キー）
			 * @param quarterTurns 回す回数（+1 で反時計回りに90度、-1 で時計回りに90度）
			 * @details 矩形の物（RectGimmick）は、中心を軸に幅と高さを入れ替える。
			 *          向きを持つ物（光源・反射鏡）は角度を回す。点や線で置く新しいギミックを作ったら override する。
			 */
			virtual void Rotate90(int quarterTurns) {}
			/**
			 * @brief 見取り図でドラッグして形を変える「取っ手」の数（ない物は 0）
			 * @details 選んでいる間、取っ手が四角い点で表示され、ドラッグすると SetEditHandle が呼ばれる。
			 *          （例：暗がりの斜めの端の上の角・端の真ん中）
			 */
			virtual int GetEditHandleCount() const { return 0; }
			/** @brief 取っ手の位置（ワールド） */
			virtual math::Vec2 GetEditHandle(int index) const { return math::Vec2::Zero(); }
			/** @brief 取っ手を position までドラッグしたときに、形を変える */
			virtual void SetEditHandle(int index, const math::Vec2& position) {}


		public:
			// ---- ファイル ----
			/**
			 * @brief JSON への書き出し・JSON からの読み込み（同じ関数で両方を行う）
			 * @details archive.Field("openSpeed", openSpeed); のように、保存したい値を並べる。
			 *          "type"（クラス名）と "name" はステージの読み込み側が書くので、ここでは書かない。
			 *          読み込みのときは JSON に書いてある値だけ上書きし、無い値は初期値のまま。
			 */
			virtual void Serialize(GimmickArchive& archive) = 0;
			/** @brief どちらのファイルに保存するか（ふつうはギミックのファイル） */
			virtual StageFileKind GetFileKind() const { return StageFileKind::Gimmick; }


		public:
			explicit GimmickBase(const char* name) : name(name != nullptr ? name : "") {}
			virtual ~GimmickBase() = default;
			GimmickBase(const GimmickBase&) = default;
			GimmickBase& operator=(const GimmickBase&) = default;
		};
	}
}
