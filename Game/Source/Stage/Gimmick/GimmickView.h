/**
 * @file GimmickView.h
 * @brief 全ギミックのリストから、ある種類だけを取り出して for 文で回すための入れ物
 * @details 例：for (const Door& door : stage.GetGimmicks().View<Door>()) { ... }
 *          リストをコピーせず、回しながら種類が違う物を飛ばしていく（dynamic_cast で種類を調べる）。
 */
#pragma once
#include <cstddef>


namespace app
{
	namespace stage
	{
		/**
		 * @brief ある種類のギミックだけを回す
		 * @tparam T 取り出す種類（const 付きなら書き換えられない）
		 * @tparam Iterator 全ギミックのリスト（unique_ptr の vector）のイテレーター
		 */
		template<class T, class Iterator>
		class GimmickView
		{
		public:
			class iterator
			{
			public:
				iterator(const Iterator current, const Iterator end)
					: m_current(current)
					, m_end(end)
				{
					SkipOtherTypes();
				}

				T& operator*() const { return static_cast<T&>(**m_current); }
				T* operator->() const { return &**this; }
				iterator& operator++()
				{
					++m_current;
					SkipOtherTypes();
					return *this;
				}
				bool operator==(const iterator& other) const { return m_current == other.m_current; }
				bool operator!=(const iterator& other) const { return m_current != other.m_current; }

			private:
				/** 種類が違う物を飛ばす */
				void SkipOtherTypes()
				{
					while (m_current != m_end && dynamic_cast<T*>(m_current->get()) == nullptr)
					{
						++m_current;
					}
				}

			private:
				Iterator m_current;
				Iterator m_end;
			};


		public:
			GimmickView(const Iterator begin, const Iterator end)
				: m_begin(begin)
				, m_end(end)
			{}

			iterator begin() const { return iterator(m_begin, m_end); }
			iterator end() const { return iterator(m_end, m_end); }
			/** 1つもないか */
			bool empty() const { return begin() == end(); }
			/** いくつあるか（数えるので、毎回全部回る） */
			size_t size() const
			{
				size_t count = 0;
				for (iterator it = begin(); it != end(); ++it) { ++count; }
				return count;
			}

		private:
			Iterator m_begin;
			Iterator m_end;
		};
	}
}
