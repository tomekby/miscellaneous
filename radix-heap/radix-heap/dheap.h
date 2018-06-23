#pragma once
#include <utility>

typedef unsigned uint_t;

/**
	* Klasa abstrakcyjna reprezentuj¹ca kopiec a-arny
	*/
template <uint_t D = 2>
class dheap {
public:
	// Struktura przechowuj¹ca element kolejki
	struct element {
		element() : value(0), key(0) {}

		element(const uint_t value, const uint_t key) : value(value), key(key) {}

		uint_t value;
		uint_t key;
	};

	explicit dheap(const uint_t size) : _size(size), _count(size) {
		_elements = new element[_size];
		_positions = new size_t[_size];
	}

	~dheap() {
		delete[] _elements;
		delete[] _positions;
	}

	void push(const uint_t value, const uint_t key) {
		_elements[value] = element(value, key);
		_positions[value] = value;
	}

	void build_heap() {
		for (int i = _size / D; i >= 0; --i) move_down(i);
	}

	const element& front() const {
		if (_elements[0].key == 0u) return _elements[pos(0)];
		return _elements[0];
	}

	uint_t get_key(const uint_t value) {
		return _elements[pos(value)].key;
	}

	void change_priority(const uint_t value, const uint_t new_prio) {
		const auto position = pos(value);
		const auto old_key = _elements[position].key;
		_elements[position].key = new_prio;
		fix_heap(value);
	}

	const element& pop() {
		auto res = _elements[0];
		swap(0, --_count);
		_positions[res.value] = INT_MAX ;
		move_down(0);

		return res;
	}

	void fix_heap(const uint_t value) {
		move_down(pos(value));
	}

	bool in_heap(const uint_t& value) const {
		return _positions[value] != INT_MAX ;
	}

protected:
	uint_t get_parent(const uint_t id) const {
		if (id == 0) return 0;
		return (id - 1) / D;
	}

	bool cmp(const element& a, const element& b) {
		return a.key <= b.key;
	}

	uint_t& pos(const uint_t value) const {
		return _positions[value];
	}

	void swap(const uint_t i, const uint_t largest) {
		std::swap(_elements[i], _elements[largest]);
		std::swap(pos(_elements[i].value), pos(_elements[largest].value));
	}

	void move_down(const uint_t i) {
		const auto left = i * D + 1, right = left + D;
		auto extremum = i;

		const auto limit = right < _count ? right : _count;
		// Szukanie najwiêkszego/najmniejszego dziecka
		for (auto j = left; j < limit; ++j) {
			if (cmp(_elements[j], _elements[extremum])) {
				extremum = j;
			}
		}

		if (extremum ^ i) {
			swap(i, extremum);
			move_down(extremum);
		}
	}

	// Elementy
	element* _elements;
	// Pozycje elementów
	uint_t* _positions;
	// Iloœæ elementów w kolejce
	const uint_t _size;
	uint_t _count;
};