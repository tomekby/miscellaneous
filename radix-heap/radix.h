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
	@todo: coœ z tym lookup table jest zjebane, bo z³e indeksy s¹ wpisane

	czy cachowanie min_value dla poszczególnych kube³ków ma sens? szybciej nie bêdzie szukaæ minimum podczas
	usuwania minimum? i tak trzeba przeiterowaæ przez ca³y kube³ek; chyba ma, bo trzeba by go iterowaæ 2 razy
	a jeœli jest to cachowane, to tylko w przypadku pesymistycznym redukcji priorytetu bêdzie iterowanie

	po skoñczonej implementacji sprawdzenie co jest szybsze -> reduce_priority vs multiple pops vs lookup table + reduce_priority
 */

/**
 * Radix heap - "kopiec kube³kowy"
 * Dzia³a zak³adaj¹c, ¿e priorytet jest liczb¹ ca³kowit¹ >= 0
 * Pozwala na implementacjê alg. Dijkstry o z³o¿onoœci ~O(m+n*logC)
 * Przy okazji jest znacznie prostszy w implementacji ni¿ np. Kopiec Fibonacciego
 * Wersja wykorzystuj¹ca lookup table dzia³a tylko jeœli element_t jest liczb¹
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
 * Pomys³ stworzenia lookup table dla reduce_priority zaczerpniêty st¹d:
 * - http://www.cosc.canterbury.ac.nz/tad.takaoka/alg/spalgs/radixheap.txt
 * Czas dzia³ania wg.:
 * - http://ocw.mit.edu/courses/sloan-school-of-management/15-082j-network-optimization-fall-2010/lecture-notes/MIT15_082JF10_lec06.pdf
 *
 * Autor: Tomasz Stasiak
 */
template <class element_t>
class RadixHeap {
public:
	// Struktura przechowuj¹ca element kolejki
	struct element {
		element() : value(element_t()), key(0) {}
		element(const element_t& value, const size_t key) : value(value), key(key) {}
		element_t value;
		size_t key;

		static size_t min(size_t a, size_t b) {
			return (a < b) ? a : b;
		}
	};
	typedef vector<element> bucket_t;

	/**
	 * Konstruktor
	 * @param element_count iloœæ elementów (np. wierzcho³ków) przechowywanych w kopcu
	 */
	RadixHeap(const size_t element_count = 1) : _element_count(element_count) {
		_items_count = 0;
		// iloœæ kube³kow ~ iloœæ bitów maksymalnej wielkoœci klucza
		_buckets_no = static_cast<unsigned>(ceil(log2(MAX_PRIORITY)) + 1);
		_last_deleted = 0;

		// Alokacja kube³ków
		_buckets = new bucket_t[_buckets_no];
		for (size_t i = 0; i < _buckets_no; ++i) {
			_buckets[i] = bucket_t();
			_buckets[i].resize(1);
		}
		_buckets_min = new size_t[_buckets_no];
		for (size_t i = 0; i < _buckets_no; ++i) _buckets_min[i] = NON_EXISTING;

		// Inicjalizacja lookup table
		_element_positions = new size_t[element_count];
		for (size_t i = 0; i < element_count; ++i) _element_positions[i] = NON_EXISTING;
	}
	~RadixHeap() {
		delete[] _buckets;
		delete[] _buckets_min;
		delete[] _element_positions;
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
		else if (!_buckets[0].empty()) _remove_least();

		// Szukanie pierwszego, niepustego kube³ka
		size_t i = 0;
		for (; _buckets[i].empty(); ++i);
		// Usuniêty jest element o najmniejszym priorytecie z tego kube³ka
		_last_deleted = _buckets_min[i];
		_buckets_min[i] = NON_EXISTING;

		// Redystrybucja elementów
		while(_buckets[i].size() > 0) {
			const element& el = _buckets[i].pop_back();
			const size_t new_bucket = _get_bucket_no(el.key);
			_buckets[new_bucket].push_back(el);
			_buckets_min[new_bucket] = element::min(_buckets_min[new_bucket], el.key);
			// Element przenosimy na koniec kube³ka nowego
			_element_positions[el.value] = _buckets[new_bucket].size() - 1;
		}
		// W tym kube³ku nic nie ma, to mo¿na go zmniejszyæ
		_buckets[i].clear();

		return _remove_least();
	}
	// Wstawianie nowej wartoœci do kolejki
	void push(const element_t& item, const size_t key) {
		++_items_count;
		// Numer kube³ka do którego powinien wyl¹dowaæ ten element
		const size_t bucket_no = _get_bucket_no(key);
		const element new_element(item, key);
		_buckets[bucket_no].push_back(new_element);
		_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], key);
		// Aktualizacja lookup table - wstawiamy na ostatniej pozycji
		_element_positions[item] = _buckets[bucket_no].size() - 1;
	}
	size_t size() const {
		return _items_count;
	}
	size_t empty() const {
		return size() == 0;
	}
	// Sprawdzenie czy element jest w kopcu
	bool in_heap(const element_t& value) const {
		return _element_positions[value] != NON_EXISTING;
	}
	// Redukcja priorytetu elementu
	// @todo: aktualizacja w lookup table coœ nieteges
	void reduce_priority(const element_t& item, const size_t old_key, const size_t new_key) {
		// Usuwanie elementu z kolejki
		const size_t bucket_no = _get_bucket_no(old_key);
		const size_t item_pos = _element_positions[item];
		bucket_t& bucket = _buckets[bucket_no];
		// 2 mo¿liwoœci - usuwamy z koñca kube³ka, b¹dŸ z jego œrodka
		const element& last = bucket.pop_back();
		// Usuwamy ze œrodka - aktualizacja miejsca dla ostatniego elementu
		if (item_pos != bucket.size() - 1) {
			bucket[item_pos] = last;
			_element_positions[last.value] = item_pos;
		}
		_element_positions[item] = NON_EXISTING;

		// Przypadek szczególny - zmieniamy priorytet najmniejszego elementu
		// W takim przypadku trzeba odszukaæ element minimalny w tym kube³ku
		for (size_t i = 0; i < bucket.size(); ++i) {
			_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], bucket[i].key);
		}
		--_items_count;
		// Dodanie elementu jeszcze raz
		//push(item, new_key);
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
	// Usuniêcie najmniejszego elementu z kolejki i zwrócenie go
	element& _remove_least() {
		// Zwrócenie elementu najmniejszego - zawsze w zerowym kube³ku
		--_items_count;
		element& least = _buckets[0].pop_back();
		// Usuniêcie z lookup table
		_element_positions[least.value] = NON_EXISTING;
		return least;
	}

	// Maksymalna wartoœæ klucza/priorytetu
	static const size_t MAX_PRIORITY = 30000;
	// Kube³ki i dane z nimi zwi¹zane
	size_t _buckets_no;
	size_t _last_deleted;
	size_t _items_count;
	bucket_t* _buckets = nullptr;
	size_t* _buckets_min = nullptr;
	// Lookup table dla wyszukiwania w reduce_priority
	size_t* _element_positions = nullptr;
	size_t  _element_count;

	// Wartoœæ oznaczaj¹ca coœ nieistniej¹cego
	static const size_t NON_EXISTING = 0xDEADBEEF;
};