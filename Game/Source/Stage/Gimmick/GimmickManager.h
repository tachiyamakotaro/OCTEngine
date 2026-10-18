/**
 * @file GimmickManager.h
 * @brief ステージに置いた全ギミックをまとめて持つ
 * @details ・ギミックは種類に関係なく、1本のリスト（unique_ptr の vector）で持つ。
 *          ・更新・リセット・信号・当たり判定は、全ギミックの仮想関数を呼ぶだけ（種類ごとの処理は書かない）。
 *          ・ある種類だけ欲しいときは View<Door>() で取り出す（描画やサウンドなど、種類ごとの見た目を作る側で使う）。
 *          ・コピーすると中身も全部コピーされる（Clone を使う。エディタの Undo で使う）。
 */
#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "GimmickBase.h"
#include "GimmickView.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 全ギミック
		 */
		class GimmickManager
		{
		public:
			using GimmickList = std::vector<std::unique_ptr<GimmickBase>>;


		public:
			// ---- 追加・削除 ----
			/**
			 * @brief ギミックを作って追加する
			 * @details 例：auto& door = gimmicks.Add<Door>("Door1", 900.0f, 0.0f, 40.0f, 300.0f);
			 *          引数はそのクラスのコンストラクタにそのまま渡す。
			 *          返す参照は、そのギミックを消すまでずっと使える（中身は new で作っているので、追加しても動かない）。
			 */
			template<class T, class... Args>
			T& Add(Args&&... args)
			{
				std::unique_ptr<T> gimmick = std::make_unique<T>(std::forward<Args>(args)...);
				T& ref = *gimmick;
				m_gimmicks.push_back(std::move(gimmick));
				return ref;
			}
			/** @brief 作ったギミックを追加する（エディタの追加・複製で使う） */
			GimmickBase& Add(std::unique_ptr<GimmickBase> gimmick);
			/** @brief index 番目を消す */
			void Remove(size_t index);
			/** @brief 全部消す */
			void Clear() { m_gimmicks.clear(); }


		public:
			// ---- 取得 ----
			/** @brief 数 */
			size_t GetCount() const { return m_gimmicks.size(); }
			/** @brief index 番目（範囲外なら nullptr） */
			GimmickBase* Get(size_t index);
			const GimmickBase* Get(size_t index) const;
			/** @brief 全ギミックのリスト */
			const GimmickList& GetAll() const { return m_gimmicks; }
			/** @brief 何番目か（見つからなければ -1） */
			int FindIndex(const GimmickBase* gimmick) const;
			/** @brief 名前で探す（見つからなければ nullptr） */
			const GimmickBase* FindByName(const std::string& name) const;
			/**
			 * @brief ある種類だけを for 文で回す
			 * @details for (const Door& door : gimmicks.View<Door>()) { ... }
			 */
			template<class T>
			GimmickView<T, GimmickList::iterator> View() { return { m_gimmicks.begin(), m_gimmicks.end() }; }
			template<class T>
			GimmickView<const T, GimmickList::const_iterator> View() const { return { m_gimmicks.cbegin(), m_gimmicks.cend() }; }


		public:
			// ---- 更新 ----
			/** @brief phase の順番のギミックだけ更新する */
			void Update(UpdatePhase phase, const GimmickUpdateContext& context);
			/** @brief 実行時の状態を最初に戻す */
			void ResetRuntimeState();


		public:
			// ---- 信号 ----
			/** @brief 信号が ON か（同じ名前の信号を出す物のどれかが ON。名前が空なら false） */
			bool IsSignalOn(const std::string& signalName) const;
			/** @brief 信号を出す物の名前を集める（エディタのつながりの選択肢） */
			void CollectSignalNames(std::vector<std::string>& out) const;
			/** @brief 信号の名前が変わったので、全ギミックのつながりを付け替える */
			void RenameSignal(const std::string& oldName, const std::string& newName);
			/** @brief 信号を消したので、全ギミックのつながりから外す（同じ名前の信号がまだあれば何もしない） */
			void RemoveSignal(const std::string& signalName);


		public:
			// ---- 当たり判定 ----
			/** @brief box と重なる当たり判定を集める */
			void QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const;
			/** @brief box と重なる当たり判定があるか */
			bool AnyOverlap(const math::Rect& box) const;
			/** @brief box と重なる地形があるか（光の足場は含めない） */
			bool OverlapsTerrain(const math::Rect& box) const;


		public:
			// ---- エディタ ----
			/** @brief 重ならない名前を作る（"Block3" など） */
			std::string MakeUniqueName(const char* prefix) const;


		public:
			GimmickManager() = default;
			~GimmickManager() = default;
			// コピーすると中身も全部コピーする（Undo 用）
			GimmickManager(const GimmickManager& other);
			GimmickManager& operator=(const GimmickManager& other);
			GimmickManager(GimmickManager&&) = default;
			GimmickManager& operator=(GimmickManager&&) = default;


		private:
			/** 全ギミック（置いた順） */
			GimmickList m_gimmicks;
		};
	}
}
