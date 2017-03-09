#pragma once

#include "vector.h"
#include <vector>
#include <cassert>

/**
 * do zastanowienia si�:
 	lookup table z adresami wierzcho�k�w �eby nie trzeba by�o ich szuka� przy decrease-key
    zajmie od cholery wi�cej pami�ci, ale koszt decrease-key powinien spa�� do O(1) (?)
		- szukanie kube�ka: O(1)
		- szukanie elementu w kube�ku: O(1) zamiast O(n)
	w sumie na adres mo�na u�y� tablicy int�w/short�w co zaoszcz�dzi troch� pami�ci - indeks kube�ka znamy
 */

/**
 * Radix heap - "kopiec kube�kowy"
 * Dzia�a zak�adaj�c, �e priorytet jest liczb� ca�kowit� >= 0
 * Pozwala na implementacj� alg. Dijkstry o z�o�ono�ci ~O(m+n*logC)
 * Przy okazji jest znacznie prostszy w implementacji ni� np. Kopiec Fibonacciego
 *
 * Ta implementacja wykorzystuje vectory zamiast list poniewa�:
 * - vector zajmuje zauwa�alnie mniej pami�ci
 * - tablica jest przechowywana w spos�b ci�g�y, co znacznie zmniejsza czas dost�pu do danych
 * - u�ywane operacje dodawania/usuwania element�w wykonuj� si� w czasie O(1) - push_back/pop_back
 *   u�ycie memmove zamiast przepisania w wektorze nieznacznie przyspiesza resize
 * - okrojony vector w tej implementacji jest ~15x szybszy od std::vector (push_back/emplace_back):
 *   http://ideone.com/P17yA4
 * 
 * Implementowane na podstawie:
 * - http://ssp.impulsetrain.com/radix-heap.html
 * Pomys� stworzenia lookup-table dla reduce priority zaczerpni�ty st�d:
 * - http://www.cosc.canterbury.ac.nz/tad.takaoka/alg/spalgs/radixheap.txt
 * Czas dzia�ania wg.:
 * - http://ocw.mit.edu/courses/sloan-school-of-management/15-082j-network-optimization-fall-2010/lecture-notes/MIT15_082JF10_lec06.pdf
 *
 * Autor: Tomasz Stasiak
 */
template <class T>
class RadixHeap {
public:
	// Struktura przechowuj�ca element kolejki
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
		// ilo�� kube�kow ~ ilo�� bit�w maksymalnej wielko�ci klucza
		//_buckets_no = 33; -> fixed size
		_buckets_no = static_cast<unsigned>(ceil(log2(_max_key_val)) + 1);
		_last_deleted = 0;

		// Minima dla poszczeg�lnych kube�k�w ustawiamy na int_max
		_buckets_min = new size_t[_buckets_no];
		for (size_t i = 0; i < _buckets_no; ++i) _buckets_min[i] = UINT_MAX;
		// Alokacja kube�k�w
		_buckets = new bucket_t[_buckets_no];
	}
	~RadixHeap() {
		delete[] _buckets_min;
		delete[] _buckets;
	}

	/**
	 * Usuwanie minimum z kolejki
	 *
	 * Je�li jest kilka element�w minimalnych o tej samej warto�ci (priorytecie)
	 * to delete_min b�dzie je pakowa�o ci�gle do kube�ka #0 po usuni�ciu pierwszego z nich
	 * wi�c je�li co� jest w #0 mo�na pomin�� redystrybucj�
	 */
	element& pop() {
		if (!_items_count) return element();
		// Je�li co� jest w kube�ku #0 to jest to minimum
		else if (!_buckets[0].empty()) {
			--_items_count;
			return _buckets[0].pop_back();
		}

		// Szukanie pierwszego, niepustego kube�ka
		size_t i = 0;
		for (; _buckets[i].empty(); ++i);
		// Usuni�ty jest element o najmniejszym priorytecie z tego kube�ka
		_last_deleted = _buckets_min[i];
		_buckets_min[i] = UINT_MAX;

		while(_buckets[i].size() > 0) {
			const element& el = _buckets[i].pop_back();
			const size_t new_bucket = _get_bucket_no(el.key);
			_buckets[new_bucket].push_back(el);
			_buckets_min[new_bucket] = element::min(_buckets_min[new_bucket], el.key);
		}
		// W tym kube�ku nic nie ma, to mo�na go zmniejszy�
		_buckets[i].clear();

		--_items_count;
		// Zwr�cenie elementu najmniejszego - zawsze w zerowym kube�ku
		return _buckets[0].pop_back();
	}
	// Wstawianie nowej warto�ci do kolejki
	void push(const T& item, const size_t key) {
		++_items_count;
		// Numer kube�ka do kt�rego powinien wyl�dowa� ten element
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
	// @todo: po sko�czonej implementacji sprawdzenie co jest szybsze -> reduce_priority vs multiple pops
	// @todo: je�li element dla kt�rego zmniejszamy priorytet ma najmniejsz� warto��, to go update min w tym
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
	 * Wypisywanie zawarto�ci poszczeg�lnych kube�k�w
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
	// Numer kube�ka
	size_t _get_bucket_no(const size_t key) {
		// __lzcnt -> count trailing zeroes
		return (key == _last_deleted) ? 0 : __lzcnt(key ^ _last_deleted) + 1;
		// __lzcnt jest nieznacznie szybsze, ale wymaga obs�ugi SSE4
		/*unsigned long res;
		_BitScanReverse(&res, key ^ _last_deleted);
		return res;*/
	}

	// Maksymalna warto�� klucza/priorytetu
	static const size_t _max_key_val = 10000;
	size_t _buckets_no;
	size_t _last_deleted;
	size_t _items_count;
	bucket_t* _buckets = nullptr;
	size_t* _buckets_min = nullptr;
};