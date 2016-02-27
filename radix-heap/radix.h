#pragma once

#include "vector.h"
#include <cassert>

/**
 * do zastanowienia si�:
 	lookup table z adresami wierzcho�k�w �eby nie trzeba by�o ich szuka� przy decrease-key
    zajmie od cholery wi�cej pami�ci, ale koszt decrease-key powinien spa�� do O(1) (?)
		- szukanie kube�ka: O(1)
		- szukanie elementu w kube�ku: O(1) zamiast O(n)
	w sumie na adres mo�na u�y� tablicy int�w/short�w co zaoszcz�dzi troch� pami�ci - indeks kube�ka znamy

	czy cachowanie min_value dla poszczeg�lnych kube�k�w ma sens? szybciej nie b�dzie szuka� minimum podczas
	usuwania minimum? i tak trzeba przeiterowa� przez ca�y kube�ek; chyba ma, bo trzeba by go iterowa� 2 razy
	a je�li jest to cachowane, to tylko w przypadku pesymistycznym redukcji priorytetu b�dzie iterowanie

	po sko�czonej implementacji sprawdzenie co jest szybsze -> reduce_priority vs multiple pops vs lookup table + reduce_priority

	potrzebna b�dzie dodatkowa tablica (osobno?) z priorytetami dla poszczeg�lnych element�w (random access)
 */

/**
 * Radix heap - "kopiec kube�kowy"
 * Dzia�a zak�adaj�c, �e priorytet jest liczb� ca�kowit� >= 0
 * Pozwala na implementacj� alg. Dijkstry o z�o�ono�ci ~O(m+n*logC)
 * Przy okazji jest znacznie prostszy w implementacji ni� np. Kopiec Fibonacciego
 * Wersja wykorzystuj�ca lookup table dzia�a tylko je�li value_t jest liczb� ca�kowit�
 *
 * Ta implementacja wykorzystuje vectory zamiast list poniewa�:
 * - vector zajmuje zauwa�alnie mniej pami�ci (wielko�� jest regulowana automatycznie)
 * - tablica jest przechowywana w spos�b ci�g�y, co znacznie zmniejsza czas dost�pu do danych
 * - u�ywane operacje dodawania/usuwania element�w wykonuj� si� w czasie O(1) - push_back/pop_back
 * - okrojony vector w tej implementacji jest znacznie szybszy od std::vector (push_back/emplace_back)
 * 
 * Implementowane na podstawie:
 * - http://ssp.impulsetrain.com/radix-heap.html
 * Pomys� stworzenia lookup table dla redukcji priorytetu zaczerpni�ty st�d:
 * - http://www.cosc.canterbury.ac.nz/tad.takaoka/alg/spalgs/radixheap.txt
 * Czas dzia�ania wg.:
 * - http://ocw.mit.edu/courses/sloan-school-of-management/15-082j-network-optimization-fall-2010/lecture-notes/MIT15_082JF10_lec06.pdf
 *
 * Autor: Tomasz Stasiak
 */
template <class value_t, class key_t>
class RadixHeap {
public:
	// Struktura przechowuj�ca element kolejki
	struct element {
		element() : value(value_t()), key(0) {}
		element(const value_t& value, const key_t key) : value(value), key(key) {}
		value_t value;
		key_t key;

		static key_t min(key_t a, key_t b) {
			return (a < b) ? a : b;
		}
	};
	typedef vector<element> bucket_t;
	// Typ przechowuj�cy miejsce w kube�ku dla poszczceg�lnych warto�ci
	// Je�li element�w jest ma�o, mo�e by� mniejszy typ - mniej pami�ci
	typedef size_t position_t;

	/**
	 * Konstruktor
	 * @param element_count maksymalna ilo�� element�w (np. wierzcho�k�w) przechowywanych w kopcu
	 */
	RadixHeap(const position_t element_count = 1) : _element_count(element_count) {
		_items_count = 0;
		// ilo�� kube�kow ~ ilo�� bit�w maksymalnej wielko�ci klucza
		_buckets_no = static_cast<size_t>(ceil(log2(MAX_PRIORITY)) + 1);
		_last_deleted = 0;

		// Alokacja kube�k�w
		_buckets = new bucket_t[_buckets_no];
		_buckets_min = new key_t[_buckets_no];
		for (size_t i = 0; i < _buckets_no; ++i) _buckets_min[i] = NON_EXISTING_KEY;

		// Inicjalizacja lookup table
		_element_positions = new position_t[element_count];
		for (position_t i = 0; i < element_count; ++i) _element_positions[i] = NON_EXISTING_POS;
	}
	// Destruktor
	~RadixHeap() {
		delete[] _buckets;
		delete[] _buckets_min;
		delete[] _element_positions;
	}

	/**
	 * Usuwanie minimum z kolejki
	 *
	 * Je�li jest kilka element�w minimalnych o tej samej warto�ci (priorytecie)
	 * to delete_min b�dzie je pakowa�o ci�gle do kube�ka #0 po usuni�ciu pierwszego z nich
	 * wi�c je�li co� jest w #0 mo�na pomin�� redystrybucj�
	 *
	 * Zwraca najmniejszy element w kolejce
	 */
	element& pop() {
		if (!_items_count) return element();
		// Je�li co� jest w kube�ku #0 to jest to minimum
		else if (!_buckets[0].empty()) _remove_least();

		// Szukanie pierwszego, niepustego kube�ka
		size_t i = 0;
		for (; _buckets[i].empty(); ++i);
		// Usuni�ty jest element o najmniejszym priorytecie z tego kube�ka
		_last_deleted = _buckets_min[i];
		_buckets_min[i] = NON_EXISTING_KEY;

		// Redystrybucja element�w
		while(_buckets[i].size() > 0) {
			const element& el = _buckets[i].pop_back();
			const key_t new_bucket = _get_bucket_no(el.key);
			_buckets[new_bucket].push_back(el);
			_buckets_min[new_bucket] = element::min(_buckets_min[new_bucket], el.key);
			// Element przenosimy na koniec nowego kube�ka
			_element_positions[el.value] = _buckets[new_bucket].size() - 1;
		}

		return _remove_least();
	}

	/**
	 * Wstawianie nowej warto�ci do kolejki
	 *
	 * @param item warto�� do wstawienia
	 * @param key priorytet elementu do wstawienia
	 */
	void push(const value_t& item, const key_t key) {
		assert(MAX_PRIORITY >= key);

		++_items_count;
		// Numer kube�ka do kt�rego powinien wyl�dowa� ten element
		const key_t bucket_no = _get_bucket_no(key);
		const element new_element(item, key);
		_buckets[bucket_no].push_back(new_element);
		_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], key);
		// Aktualizacja lookup table - wstawiamy na ostatniej pozycji
		_element_positions[item] = _buckets[bucket_no].size() - 1;
	}

	/**
	 * Zmiana priorytetu elementu b�d�cego ju� w kolejce
	 *
	 * @param item warto�� dla kt�rej ma by� zmieniony priorytet
	 * @param old_key poprzedni priorytet dla elementu
	 * @param new_key nowy priorytet dla elementu
	 */
	void change_priority(const value_t& item, const key_t old_key, const key_t new_key) {
		// Usuwanie elementu z kolejki
		const key_t bucket_no = _get_bucket_no(old_key);
		const position_t item_pos = _element_positions[item];
		// 2 mo�liwo�ci - usuwamy z ko�ca kube�ka, b�d� z jego �rodka
		const element& last = _buckets[bucket_no].pop_back();
		// Usuwamy ze �rodka - aktualizacja miejsca dla ostatniego elementu
		if (item_pos != _buckets[bucket_no].size() - 1) {
			_buckets[bucket_no][item_pos] = last;
			_element_positions[last.value] = item_pos;
		}
		_element_positions[item] = NON_EXISTING_POS;

		// Przypadek szczeg�lny - zmieniamy priorytet najmniejszego elementu
		// W takim przypadku trzeba odszuka� element minimalny w tym kube�ku
		_fix_minimum(bucket_no, old_key);

		// Dodanie elementu jeszcze raz
		--_items_count;
		push(item, new_key);
	}

	// Ilo�� element�w w kolejce
	size_t size() const {
		return _items_count;
	}
	// Czy kolejka jest pusta?
	bool empty() const {
		return size() == 0;
	}
	// Sprawdzenie czy element jest w kopcu
	bool in_heap(const value_t& value) const {
		return _element_positions[value] != NON_EXISTING_POS;
	}
	/**
	 * Wypisywanie zawarto�ci poszczeg�lnych kube�k�w
	 */
	void dump() {
		printf("zawartosc kolejki (priorytet, wartosc, miejsce):\n");
		for (key_t i = 0; i < _buckets_no; ++i) {
			printf("#%d\t", i);
			if (_buckets_min[i] == NON_EXISTING_KEY) {
				printf("pusto\n");
				continue;
			}
			for (position_t j = 0; j < _buckets[i].size(); ++j) {
				printf("(%d, %d, %d), ", _buckets[i][j].key, _buckets[i][j].value, _element_positions[_buckets[i][j].value]);
			}
			printf("\n");
		}
		printf("\n");
	}
private:
	// Numer kube�ka jest okre�lany przez najwy�szy ustawiony 
	// __lzcnt jest nieznacznie szybsze od _BitScanReverse, ale wymaga obs�ugi SSE4
	key_t _get_bucket_no(const key_t key) {
		if (key == _last_deleted) return 0;

		key_t res = __lzcnt(key ^ _last_deleted);
		//unsigned long res;
		//_BitScanReverse(&res, key ^ _last_deleted);
		assert(res + 1 < _buckets_no);
		return res + 1;
	}

	/**
	 * Usuni�cie najmniejszego elementu z kolejki i zwr�cenie go
	 * Zwraca najmniejszy element (wcze�niej umieszczony w kube�ku #0)
	 */
	element& _remove_least() {
		// Zwr�cenie elementu najmniejszego - zawsze w zerowym kube�ku
		--_items_count;
		element& least = _buckets[0].pop_back();
		// Usuni�cie z lookup table
		_element_positions[least.value] = NON_EXISTING_POS;
		return least;
	}
	/**
	 * Szukanie nowego minimum dla okre�lonego kube�ka
	 * Je�li usuni�ta warto�� nie by�a minimum w kube�ku, funkcja nic nie robi
	 *
	 * @param bucket_no numer kube�ka dla kt�rego ma by� poprawione minimum
	 * @param old_key usuni�ta warto��
	 */
	void _fix_minimum(const key_t bucket_no, const key_t old_key) {
		if (_buckets_min[bucket_no] == old_key) {
			_buckets_min[bucket_no] = NON_EXISTING_KEY;
			for (position_t i = 0; i < _buckets[bucket_no].size(); ++i) {
				_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], _buckets[bucket_no][i].key);
			}
		}
	}

	// Maksymalna warto�� klucza/priorytetu
	static const key_t MAX_PRIORITY = UINT_MAX;
	// Warto�� oznaczaj�ca co� nieistniej�cego
	static const key_t NON_EXISTING_KEY = std::numeric_limits<key_t>::max();
	static const position_t NON_EXISTING_POS = std::numeric_limits<position_t>::max();

	// Kube�ki i dane z nimi zwi�zane
	key_t _buckets_no;
	// Ostatnio usuni�ta warto��
	key_t _last_deleted;
	// Ilo�� element�w w kolejce
	position_t _items_count;
	bucket_t* _buckets = nullptr;
	// Minima dla poszczeg�lnych kube�k�w
	key_t* _buckets_min = nullptr;

	// Lookup table dla wyszukiwania w reduce_priority
	position_t* _element_positions = nullptr;
	// Wielko�� lookup table
	position_t _element_count;
};