#pragma once

#include "vector.h"
#include <vector>
#include <cassert>

/**
 * do zastanowienia siê:
 	lookup table z adresami wierzcho³ków ¿eby nie trzeba by³o ich szukaæ przy decrease-key
    zajmie od cholery wiêcej pamiêci, ale koszt decrease-key powinien spaœæ do O(1) (?)
		- szukanie kube³ka: O(1)
		- szukanie elementu w kube³ku: O(1) zamiast O(n)
	w sumie na adres mo¿na u¿yæ tablicy intów/shortów co zaoszczêdzi trochê pamiêci - indeks kube³ka znamy
 */

/**
 * Radix heap - "kopiec kube³kowy"
 * Dzia³a zak³adaj¹c, ¿e priorytet jest liczb¹ ca³kowit¹ >= 0
 * Pozwala na implementacjê alg. Dijkstry o z³o¿onoœci ~O(m+n*logC)
 * Przy okazji jest znacznie prostszy w implementacji ni¿ np. Kopiec Fibonacciego
 *
 * Ta implementacja wykorzystuje vectory zamiast list poniewa¿:
 * - vector zajmuje zauwa¿alnie mniej pamiêci
 * - tablica jest przechowywana w sposób ci¹g³y, co znacznie zmniejsza czas dostêpu do danych
 * - u¿ywane operacje dodawania/usuwania elementów wykonuj¹ siê w czasie O(1) - push_back/pop_back
 *   u¿ycie memmove zamiast przepisania w wektorze nieznacznie przyspiesza resize
 * - okrojony vector w tej implementacji jest ~15x szybszy od std::vector (push_back/emplace_back):
 *   http://ideone.com/P17yA4
 * 
 * Implementowane na podstawie:
 * - http://ssp.impulsetrain.com/radix-heap.html
 * Pomys³ stworzenia lookup-table dla reduce priority zaczerpniêty st¹d:
 * - http://www.cosc.canterbury.ac.nz/tad.takaoka/alg/spalgs/radixheap.txt
 * Czas dzia³ania wg.:
 * - http://ocw.mit.edu/courses/sloan-school-of-management/15-082j-network-optimization-fall-2010/lecture-notes/MIT15_082JF10_lec06.pdf
 *
 * Autor: Tomasz Stasiak
 */
template <class T>
class RadixHeap {
public:
	// Struktura przechowuj¹ca element kolejki
	struct element {
		element() : value(T()), key(0) {}
		element(const T& value, const size_t key) : value(value), key(key) {}
		T value;
		size_t key;

		static size_t min(size_t a, size_t b) {
			return (a < b) ? a : b;
		}
	};
	typedef vector<element> bucket_t;

	/**
	 * Konstruktor
	 */
	RadixHeap() {
		_items_count = 0;
		// iloœæ kube³kow ~ iloœæ bitów maksymalnej wielkoœci klucza
		//_buckets_no = 33; -> fixed size
		_buckets_no = static_cast<unsigned>(ceil(log2(_max_key_val)) + 1);
		_last_deleted = 0;

		// Minima dla poszczególnych kube³ków ustawiamy na int_max
		_buckets_min = new size_t[_buckets_no];
		for (size_t i = 0; i < _buckets_no; ++i) _buckets_min[i] = UINT_MAX;
		// Alokacja kube³ków
		_buckets = new bucket_t[_buckets_no];
	}
	~RadixHeap() {
		delete[] _buckets_min;
		delete[] _buckets;
	}

	/**
	 * Usuwanie minimum z kolejki
	 *
	 * Jeœli jest kilka elementów minimalnych o tej samej wartoœci (priorytecie)
	 * to delete_min bêdzie je pakowa³o ci¹gle do kube³ka #0 po usuniêciu pierwszego z nich
	 * wiêc jeœli coœ jest w #0 mo¿na pomin¹æ redystrybucjê
	 */
	element& pop() {
		if (!_items_count) return element();
		// Jeœli coœ jest w kube³ku #0 to jest to minimum
		else if (!_buckets[0].empty()) {
			--_items_count;
			return _buckets[0].pop_back();
		}

		// Szukanie pierwszego, niepustego kube³ka
		size_t i = 0;
		for (; _buckets[i].empty(); ++i);
		// Usuniêty jest element o najmniejszym priorytecie z tego kube³ka
		_last_deleted = _buckets_min[i];
		_buckets_min[i] = UINT_MAX;

		while(_buckets[i].size() > 0) {
			const element& el = _buckets[i].pop_back();
			const size_t new_bucket = _get_bucket_no(el.key);
			_buckets[new_bucket].push_back(el);
			_buckets_min[new_bucket] = element::min(_buckets_min[new_bucket], el.key);
		}
		// W tym kube³ku nic nie ma, to mo¿na go zmniejszyæ
		_buckets[i].clear();

		--_items_count;
		// Zwrócenie elementu najmniejszego - zawsze w zerowym kube³ku
		return _buckets[0].pop_back();
	}
	// Wstawianie nowej wartoœci do kolejki
	void push(const T& item, const size_t key) {
		++_items_count;
		// Numer kube³ka do którego powinien wyl¹dowaæ ten element
		const size_t bucket_no = _get_bucket_no(key);
		const element new_element(item, key);
		_buckets[bucket_no].push_back(new_element);
		_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], key);
	}
	size_t size() const {
		return _items_count;
	}
	size_t empty() const {
		return size() == 0;
	}
	// Redukcja priorytetu elementu
	// @todo: po skoñczonej implementacji sprawdzenie co jest szybsze -> reduce_priority vs multiple pops
	// @todo: jeœli element dla którego zmniejszamy priorytet ma najmniejsz¹ wartoœæ, to go update min w tym
	void reduce_priority(const T& item, const size_t old_key, const size_t new_key) {
		// Usuwanie elementu z kolejki
		bucket_t& bucket = _buckets[_get_bucket_no(old_key)];
		for (size_t i = 0; i < bucket.size(); ++i) {
			if (bucket[i].value == item && bucket[i].key == old_key) {
				if (i == bucket.size() - 1) bucket->pop_back();
				else bucket[i] = bucket->pop_back();
				break;
			}
		}
		// Dodanie go jeszcze raz
		insert(item, new_key);
	}
	/**
	 * Wypisywanie zawartoœci poszczególnych kube³ków
	 */
	void dump() {
		printf("zawartosc kolejki (priorytet, wartosc):\n");
		for (size_t i = 0; i < _buckets_no; ++i) {
			printf("#%d\t", i);
			if (_buckets_min[i] == UINT_MAX) {
				printf("pusto\n");
				continue;
			}
			for (size_t j = 0; j < _buckets[i].size(); ++j) {
				printf("(%d, %d), ", _buckets[i][j].key, _buckets[i][j].value);
			}
			printf("\n");
		}
		printf("\n");
	}
private:
	// Numer kube³ka
	size_t _get_bucket_no(const size_t key) {
		// __lzcnt -> count trailing zeroes
		return (key == _last_deleted) ? 0 : __lzcnt(key ^ _last_deleted) + 1;
		// __lzcnt jest nieznacznie szybsze, ale wymaga obs³ugi SSE4
		/*unsigned long res;
		_BitScanReverse(&res, key ^ _last_deleted);
		return res;*/
	}

	// Maksymalna wartoœæ klucza/priorytetu
	static const size_t _max_key_val = 10000;
	size_t _buckets_no;
	size_t _last_deleted;
	size_t _items_count;
	bucket_t* _buckets = nullptr;
	size_t* _buckets_min = nullptr;
};