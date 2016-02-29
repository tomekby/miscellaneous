#pragma once

#include <vector>
#include <array>
#include <cstddef>
#include <limits>
#include <map>
#include <type_traits>

namespace radix {
	/**
	 * Radix heap
	 *
	 * Wymagania:
	 *	- klucz jest intem
	 *	- elementy wrzucane s¹ nie mniejsze ni¿ ostatni usuniêty element
	 */
	template <
		class value_t,
		class key_t = unsigned,
		class bucket_t = std::vector<std::pair<value_t, key_t>>
	>
	class queue {
		static_assert(std::is_integral<key_t>::value, "Key must be integer type");
	public:
		typedef std::pair<value_t, key_t> element_t;

		queue() : _items_count(0), _last_deleted(0) {
			for (size_t i = 0; i < _buckets_min.size(); ++i) _buckets_min[i] = MAX_PRIORITY;
		}

		virtual element_t& pop() {
			if (!_buckets[0].empty()) return _remove_least();

			size_t i = 0;
			for (; _buckets[i].empty(); ++i);

			_last_deleted = _buckets_min[i];
			_buckets_min[i] = MAX_PRIORITY;

			while (!_buckets[i].empty()) _redistribute_bucket(i);

			return _remove_least();
		}

		virtual void push(const value_t& item, const key_t key) {
			++_items_count;

			const key_t bucket = _find_bucket(key);
			_buckets[bucket].emplace_back(item, key);
			_buckets_min[bucket] = std::min(_buckets_min[bucket], key);
		}

		size_t size() const {
			return _items_count;
		}

		bool empty() const {
			return _items_count == 0;
		}
	protected:
		key_t _find_bucket(const key_t key) const {
			if (key == _last_deleted) return 0;
			// @todo: gcc version
			return 1;// __lzcnt(key ^ _last_deleted);
		}

		virtual element_t& _remove_least() {
			--_items_count;
			element_t& least = _buckets[0].back();
			_buckets[0].pop_back();
			return least;
		}

		virtual key_t _redistribute_bucket(const size_t i) {
			const element_t& el = _buckets[i].back();
			const key_t new_bucket = _find_bucket(el.second);
			_buckets[i].pop_back();
			_buckets[new_bucket].emplace_back(el.first, el.second);
			_buckets_min[new_bucket] = std::min(_buckets_min[new_bucket], el.second);

			return new_bucket;
		}

		// Iloœæ kube³ków
		static const size_t BUCKETS_COUNT = 32;
		// Maksymalna wartoœæ klucza/priorytetu
		static const key_t MAX_PRIORITY = std::numeric_limits<key_t>::max();
		// Wartoœæ oznaczaj¹ca nieistniej¹cy klucz (priorytet)
		static const key_t NON_EXISTING_KEY = std::numeric_limits<key_t>::max();

		// Ostatnio usuniêta wartoœæ
		key_t _last_deleted;
		// Iloœæ elementów w kolejce
		size_t _items_count;
		std::array<bucket_t, BUCKETS_COUNT> _buckets;
		std::array<key_t, BUCKETS_COUNT> _buckets_min;
	};

	/**
	 * Radix heap wykorzystuj¹cy lookup table
	 * Potencjalnie szybsza wersja przy wykorzystaniu dla alg. Dijkstry
	 * Przy okazji pozwala na zmianê priorytetu elementów bêd¹cych w kolejce
	 *
	 * Wymagania:
	 *	- klucz jest intem
	 *	- elementy wrzucane s¹ nie mniejsze ni¿ ostatni usuniêty element
	 *  - wartoœci (nie klucze!) s¹ unikalne
	 */
	 // Wersja wykorzystuj¹ca lookup table
	template <
		class value_t,
		class key_t = unsigned,
		class bucket_t = std::vector<std::pair<value_t, key_t>>
	>
	class lookup_heap : queue<value_t, key_t, bucket_t> {
		typedef queue<value_t, key_t, bucket_t> parent_t;
		typedef std::pair<size_t, key_t> _pos_prio_t;
		typedef typename parent_t::element_t element_t;
	public:
		virtual void push(const value_t& item, const key_t key) {
			parent_t::push(item, key);
			const key_t bucket = _find_bucket(key);
			_pos_prio[item] = _pos_prio_t(this->_buckets[bucket].size() - 1, key);
		}

		void change_priority(const value_t& item, const key_t new_key) {
			// Poprzednie dane elementu
			const _pos_prio_t& prev_pos_prio = _pos_prio[item];
			const key_t old_key = prev_pos_prio.second;
			const size_t item_pos = prev_pos_prio.first;
			const bucket_t& bucket = this->_buckets[_find_bucket(prev_pos_prio.second)];
			if (item_pos != bucket.size() - 1) {
				_pos_prio[bucket.back().first].first = item_pos;
				std::swap(bucket[item_pos], this->_buckets[bucket].back());
			}
			bucket.pop_back();
		}

		bool in_heap(const value_t& item) const {
			return _pos_prio.find(item) != _pos_prio.end();
		}
	protected:
		virtual key_t _redistribute_bucket(const size_t i) {
			const element_t& el = this->_buckets[i].back();
			const key_t new_bucket = _redistribute_bucket(i);
			_pos_prio[el.first].first = this->_buckets[new_bucket].size() - 1;
		}

		void _fix_minimum(const key_t bucket_no, const key_t old_key, const key_t new_key) {
			if (this->_buckets_min[bucket_no] != old_key || _find_bucket(old_key) == _find_bucket(new_key))
				return;
			const key_t old_min = this->_buckets_min[bucket_no];
			this->_buckets_min[bucket_no] = this->MAX_PRIORITY;
			const bucket_t& bucket = this->_buckets[bucket_no];
			for (size_t i = 0; i < bucket.size(); ++i) {
				if (old_min == bucket[i].second) {
					this->_buckets_min[bucket_no] = old_min;
					return;
				}
				this->_buckets_min[bucket_no] = std::min(this->_buckets_min[bucket_no], bucket[i].second);
			}
		}

		virtual element_t& _remove_least() {
			const element_t& least = parent_t::_remove_least();
			_pos_prio[least.first] = _pos_prio_t(NON_EXISTING_POS, this->NON_EXISTING_KEY);
			return least;
		}

		std::map<value_t, _pos_prio_t> _pos_prio;
		static const size_t NON_EXISTING_POS = std::numeric_limits<size_t>::max();
	};

	// @todo: wersja lookup z tablic¹ zamiast mapy (?)
}