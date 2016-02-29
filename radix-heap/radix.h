#pragma once

#include "vector.h"
#include <cassert>

/**
 * Czy ma by� u�yte cachowanie pozycji i priorytet�w?
 * Poch�ania to wi�ksz� ilo�� pami�ci i mo�e nie by� wymagane je�li nie trzeba modyfikowa� priorytetu element�w.
 * Mo�e nieznacznie (~5-10%) spowalnia� kolejk� (push/pop bez redukcji priorytetu), ale umo�liwia zmian�
 * priorytetu w czasie mniej-wi�cej sta�ym. Dodatkowo, cachowanie wymusza wrzucanie do kolejki wy��cznie
 * unikalnych warto�ci (przez u�ycie trywialnej funkcji hashuj�cej)
 */
#ifndef USE_LOOKUP_TABLES
	#define USE_LOOKUP_TABLES 1
#endif
/**
 * Czy u�ywa� __lzcnt() wymagaj�cego obs�ugi SSE4 przez procesor
 * Je�li 0, b�dzie u�yte nieznacznie wolniejsze _BitScanReverse()
 */
#ifndef USE_SSE4
	#define USE_SSE4 1
#endif
/**
 * Czy Szukanie minimum w kube�ku ma si� odbywa� przy ka�dej operacji (b�dzie cachowane dla pop()),
 * czy tylko w pop()
 * Przy pewnych danych wy��czenie mo�e przyspieszy�, m.in. gdy jest wiele pesymistycznych przypadk�w
 * reduce_priority (przenosimy minimum do innego kube�ka)
 * Wy��czenie gdy USE_LOOKUP_TABLES == 0 nie ma sensu, bo oszcz�dzi niewiele pami�ci a spowolni pop()
 */
#ifndef CACHE_MIN
	#define CACHE_MIN 1
#endif

/**
 * Radix heap - "kopiec kube�kowy"
 * Dzia�a zak�adaj�c, �e priorytet jest liczb� ca�kowit� >= 0
 * Pozwala na implementacj� alg. Dijkstry o z�o�ono�ci ~O(m+n*logC)
 * Przy okazji jest znacznie prostszy w implementacji ni� np. Kopiec Fibonacciego
 * Wersja wykorzystuj�ca lookup table (USE_LOOKUP_TABLES = 1) dzia�a tylko je�li value_t
 * jest liczb� ca�kowit� ca�kowit� (np. numer wierzcho�ka w alg. Dijkstry)
 *
 * Ww. dyrektywy u�ywane s� do warunkowej kompilacji kodu dzi�ki czemu mo�na go dostosowa�
 * w zale�no�ci od wymaga� (zu�ycie pami�ci/szybko��). W��czenie obu rodzaj�w cache mo�e
 * znacznie przyspieszy� dzia�anie kodu, ALE powoduje zu�ycie ~40% wi�cej pami�ci
 *
 * Ta implementacja wykorzystuje vectory zamiast list poniewa�:
 * - vector zajmuje zauwa�alnie mniej pami�ci (wielko�� jest regulowana automatycznie)
 * - tablica jest przechowywana w spos�b ci�g�y, co znacznie zmniejsza czas dost�pu do danych
 * - u�ywane operacje dodawania/usuwania element�w wykonuj� si� w czasie O(1) - push_back/pop_back
 * - okrojony vector w tej implementacji jest znacznie szybszy od std::vector (push_back/emplace_back)
 * - testy wg. Visuala: http://ideone.com/wxwrwg
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
	// Typ okre�laj�cy poszczeg�lne kube�ki
	typedef vector<element> bucket_t;
#if USE_LOOKUP_TABLES
	// Typ przechowuj�cy miejsce w kube�ku dla poszczceg�lnych warto�ci
	// Je�li element�w jest ma�o (<2^8/2^16), mo�e by� mniejszy typ - mniej pami�ci
	typedef size_t position_t;
#endif

	/**
	 * Konstruktor
	 * @param element_count maksymalna ilo�� element�w (np. wierzcho�k�w) przechowywanych w kopcu
	 */
#if USE_LOOKUP_TABLES
	RadixHeap() : _element_count(0) {}
	RadixHeap(const position_t element_count) : _element_count(element_count) {
		// Inicjalizacja lookup tables
		_element_positions = new position_t[element_count];
		_current_priorities = new key_t[element_count];
		for (position_t i = 0; i < element_count; ++i) {
			_element_positions[i] = NON_EXISTING_POS;
			_current_priorities[i] = NON_EXISTING_KEY;
		}
#else
	RadixHeap(const size_t useless = 0) {
#endif
		_items_count = 0;
		// ilo�� kube�kow ~ ilo�� bit�w maksymalnej wielko�ci klucza
		_buckets_no = static_cast<key_t>(ceil(log2(MAX_PRIORITY)) + 1);
		_last_deleted = 0;

		// Alokacja kube�k�w
		_buckets = new bucket_t[_buckets_no];
#if CACHE_MIN
		_buckets_min = new key_t[_buckets_no];
		for (size_t i = 0; i < _buckets_no; ++i) _buckets_min[i] = NON_EXISTING_KEY;
#endif
	}

	// Destruktor
	~RadixHeap() {
		delete[] _buckets;
#if CACHE_MIN
		delete[] _buckets_min;
#endif
#if USE_LOOKUP_TABLES
		delete[] _element_positions;
		delete[] _current_priorities;
#endif
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
		else if (!_buckets[0].empty()) return _remove_least();

		// Szukanie pierwszego, niepustego kube�ka
		size_t i = 0;
		for (; _buckets[i].empty(); ++i);
		// Usuni�ty jest element o najmniejszym priorytecie z tego kube�ka
#if CACHE_MIN
		_last_deleted = _buckets_min[i];
		_buckets_min[i] = NON_EXISTING_KEY;
#else
		_last_deleted = NON_EXISTING_KEY;
		for (size_t j = 0; j < _buckets[i].size(); ++j)
			_last_deleted = element::min(_buckets[i][j].key, _last_deleted);
#endif

		// Redystrybucja element�w
		while(!_buckets[i].empty()) {
			const element& el = _buckets[i].pop_back();
			const key_t new_bucket = _find_bucket(el.key);
			_buckets[new_bucket].push_back(el);
#if CACHE_MIN
			_buckets_min[new_bucket] = element::min(_buckets_min[new_bucket], el.key);
#endif
#if USE_LOOKUP_TABLES
			// Element przenosimy na koniec nowego kube�ka
			_element_positions[el.value] = _buckets[new_bucket].size() - 1;
#endif
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
		const key_t bucket_no = _find_bucket(key);
		const element new_element(item, key);
		_buckets[bucket_no].push_back(new_element);
#if CACHE_MIN
		_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], key);
#endif
#if USE_LOOKUP_TABLES
		_element_positions[item] = _buckets[bucket_no].size() - 1;
		_current_priorities[item] = key;
#endif
	}

#if USE_LOOKUP_TABLES
	/**
	 * Zmiana priorytetu elementu b�d�cego ju� w kolejce
	 *
	 * @param item warto�� dla kt�rej ma by� zmieniony priorytet
	 * @param old_key poprzedni priorytet dla elementu
	 * @param new_key nowy priorytet dla elementu
	 */
	void reduce_priority(const value_t& item, const key_t new_key) {
		// Usuwanie elementu z kolejki
		const key_t old_key = _current_priorities[item];
		const key_t bucket_no = _find_bucket(old_key);
		const position_t item_pos = _element_positions[item];
		// Je�li element pozostaje w tym kube�ku, to nie trzeba go przesuwa�
		if (bucket_no == _find_bucket(new_key)) {
			_current_priorities[item] = new_key;
			_buckets[bucket_no][item_pos].key = new_key;
			_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], new_key);
			return;
		}
		// 2 mo�liwo�ci - usuwamy z ko�ca kube�ka, b�d� z jego �rodka
		const element& last = _buckets[bucket_no].pop_back();
		// Usuwamy ze �rodka - aktualizacja miejsca dla ostatniego elementu
		if (item_pos != _buckets[bucket_no].size() - 1) {
			_buckets[bucket_no][item_pos] = last;
			_element_positions[last.value] = item_pos;
		}

#if CACHE_MIN
		// Przypadek szczeg�lny - zmieniamy priorytet najmniejszego elementu
		// W takim przypadku trzeba odszuka� element minimalny w tym kube�ku
		_fix_minimum(bucket_no, old_key, new_key);
#endif

		// Dodanie elementu jeszcze raz
		--_items_count;
		push(item, new_key);
	}
#endif

	// Ilo�� element�w w kolejce
	size_t size() const {
		return _items_count;
	}

	// Czy kolejka jest pusta?
	bool empty() const {
		return size() == 0;
	}

#if USE_LOOKUP_TABLES
	// Sprawdzenie czy element jest w kopcu
	bool in_heap(const value_t& value) const {
		return _element_positions[value] != NON_EXISTING_POS;
	}
#endif

	/**
	 * Wypisywanie zawarto�ci poszczeg�lnych kube�k�w
	 */
	void dump() {
#if USE_LOOKUP_TABLES
		printf("zawartosc kolejki (priorytet, wartosc, miejsce):\n");
#else
		printf("zawartosc kolejki (priorytet, wartosc):\n")
#endif
		for (key_t i = 0; i < _buckets_no; ++i) {
			printf("#%d\t", i);
#if CACHE_MIN
			if (_buckets_min[i] == NON_EXISTING_KEY) {
				printf("pusto\n");
				continue;
			}
#endif
			for (position_t j = 0; j < _buckets[i].size(); ++j) {
#if USE_LOOKUP_TABLES
				printf("(%d, %d, %d), ", _buckets[i][j].key, _buckets[i][j].value, _element_positions[_buckets[i][j].value]);
#else
				printf("(%d, %d), ", _buckets[i][j].key, _buckets[i][j].value);
#endif
			}
			printf("\n");
		}
		printf("\n");
	}

private:
	/**
	 * Numer kube�ka jest okre�lany przez najwy�szy ustawiony bit
	 * @param key klucz (priorytet) dla szukanego elementu
	 */
	key_t _find_bucket(const key_t key) const {
		if (key == _last_deleted) return 0;
#if USE_SSE4
		key_t res = __lzcnt(key ^ _last_deleted);
#else
		unsigned long res;
		_BitScanReverse(&res, key ^ _last_deleted);
#endif
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
#if USE_LOOKUP_TABLES
		// Usuni�cie z lookup table
		_element_positions[least.value] = NON_EXISTING_POS;
		_current_priorities[least.value] = NON_EXISTING_KEY;
#endif
		return least;
	}

#if CACHE_MIN && USE_LOOKUP_TABLES
	/**
	 * Szukanie nowego minimum dla okre�lonego kube�ka
	 * Je�li usuni�ta warto�� nie by�a minimum w kube�ku, funkcja nic nie robi
	 * Je�li po zmianie priorytetu warto�� zostaje w kube�ku, sprawdzanie jest pomini�te
	 *
	 * @param bucket_no numer kube�ka dla kt�rego ma by� poprawione minimum
	 * @param old_key usuni�ta warto��
	 * @param old_key nowy priorytet dla warto�ci
	 */
	void _fix_minimum(const key_t bucket_no, const key_t old_key, const key_t new_key) {
		if (_buckets_min[bucket_no] != old_key || _find_bucket(old_key) == _find_bucket(new_key))
			return;
		// Poprzednie minimum dla tego kube�ka
		const key_t old_min = _buckets_min[bucket_no];
		_buckets_min[bucket_no] = NON_EXISTING_KEY;
		for (position_t i = 0; i < _buckets[bucket_no].size(); ++i) {
			// Mamy kilka element�w minimalnych
			if (old_min == _buckets[bucket_no][i].key) {
				_buckets_min[bucket_no] = old_min;
				return;
			}
			_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], _buckets[bucket_no][i].key);
		}
	}
#endif

	// Maksymalna warto�� klucza/priorytetu
	static const key_t MAX_PRIORITY = UINT_MAX;
	// Warto�� oznaczaj�ca nieistniej�cy klucz (priorytet)
	static const key_t NON_EXISTING_KEY = std::numeric_limits<key_t>::max();
#if USE_LOOKUP_TABLES
	// Warto�� oznaczaj�ca nieistniej�c� pozycj� (nie ma w kolejce)
	static const position_t NON_EXISTING_POS = std::numeric_limits<position_t>::max();
#endif

	// Kube�ki i dane z nimi zwi�zane
	key_t _buckets_no;
	// Ostatnio usuni�ta warto��
	key_t _last_deleted;
	// Ilo�� element�w w kolejce
	size_t _items_count;
	bucket_t* _buckets = nullptr;
#if CACHE_MIN
	// Minima dla poszczeg�lnych kube�k�w
	key_t* _buckets_min = nullptr;
#endif

#if USE_LOOKUP_TABLES
	// Lookup table dla wyszukiwania w reduce_priority
	position_t* _element_positions = nullptr;
	// Wielko�� lookup table
	position_t _element_count;

	// Cachowane priorytety dla poszczeg�lnych warto�ci
	key_t* _current_priorities;
#endif
};