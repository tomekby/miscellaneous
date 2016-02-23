#pragma once

#include "vector.h"
#include <cassert>

/**
 * do zastanowienia siê:
 	lookup table z adresami wierzcho³ków ¿eby nie trzeba by³o ich szukaæ przy decrease-key
    zajmie od cholery wiêcej pamiêci, ale koszt decrease-key powinien spaœæ do O(1) (?)
		- szukanie kube³ka: O(1)
		- szukanie elementu w kube³ku: O(1) zamiast O(n)
	w sumie na adres mo¿na u¿yæ tablicy intów/shortów co zaoszczêdzi trochê pamiêci - indeks kube³ka znamy

	czy cachowanie min_value dla poszczególnych kube³ków ma sens? szybciej nie bêdzie szukaæ minimum podczas
	usuwania minimum? i tak trzeba przeiterowaæ przez ca³y kube³ek; chyba ma, bo trzeba by go iterowaæ 2 razy
	a jeœli jest to cachowane, to tylko w przypadku pesymistycznym redukcji priorytetu bêdzie iterowanie

	po skoñczonej implementacji sprawdzenie co jest szybsze -> reduce_priority vs multiple pops vs lookup table + reduce_priority

	potrzebna bêdzie dodatkowa tablica (osobno?) z priorytetami dla poszczególnych elementów (random access)
 */

/**
 * Radix heap - "kopiec kube³kowy"
 * Dzia³a zak³adaj¹c, ¿e priorytet jest liczb¹ ca³kowit¹ >= 0
 * Pozwala na implementacjê alg. Dijkstry o z³o¿onoœci ~O(m+n*logC)
 * Przy okazji jest znacznie prostszy w implementacji ni¿ np. Kopiec Fibonacciego
 * Wersja wykorzystuj¹ca lookup table dzia³a tylko jeœli value_t jest liczb¹ ca³kowit¹
 *
 * Ta implementacja wykorzystuje vectory zamiast list poniewa¿:
 * - vector zajmuje zauwa¿alnie mniej pamiêci (wielkoœæ jest regulowana automatycznie)
 * - tablica jest przechowywana w sposób ci¹g³y, co znacznie zmniejsza czas dostêpu do danych
 * - u¿ywane operacje dodawania/usuwania elementów wykonuj¹ siê w czasie O(1) - push_back/pop_back
 * - okrojony vector w tej implementacji jest znacznie szybszy od std::vector (push_back/emplace_back)
 * 
 * Implementowane na podstawie:
 * - http://ssp.impulsetrain.com/radix-heap.html
 * Pomys³ stworzenia lookup table dla redukcji priorytetu zaczerpniêty st¹d:
 * - http://www.cosc.canterbury.ac.nz/tad.takaoka/alg/spalgs/radixheap.txt
 * Czas dzia³ania wg.:
 * - http://ocw.mit.edu/courses/sloan-school-of-management/15-082j-network-optimization-fall-2010/lecture-notes/MIT15_082JF10_lec06.pdf
 *
 * Autor: Tomasz Stasiak
 */
template <class value_t, class key_t>
class RadixHeap {
public:
	// Struktura przechowuj¹ca element kolejki
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
	// Typ przechowuj¹cy miejsce w kube³ku dla poszczcególnych wartoœci
	// Jeœli elementów jest ma³o, mo¿e byæ mniejszy typ - mniej pamiêci
	typedef size_t position_t;

	/**
	 * Konstruktor
	 * @param element_count maksymalna iloœæ elementów (np. wierzcho³ków) przechowywanych w kopcu
	 */
	RadixHeap(const position_t element_count = 1) : _element_count(element_count) {
		_items_count = 0;
		// iloœæ kube³kow ~ iloœæ bitów maksymalnej wielkoœci klucza
		_buckets_no = static_cast<size_t>(ceil(log2(MAX_PRIORITY)) + 1);
		_last_deleted = 0;

		// Alokacja kube³ków
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
	 * Jeœli jest kilka elementów minimalnych o tej samej wartoœci (priorytecie)
	 * to delete_min bêdzie je pakowa³o ci¹gle do kube³ka #0 po usuniêciu pierwszego z nich
	 * wiêc jeœli coœ jest w #0 mo¿na pomin¹æ redystrybucjê
	 *
	 * Zwraca najmniejszy element w kolejce
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
		_buckets_min[i] = NON_EXISTING_KEY;

		// Redystrybucja elementów
		while(_buckets[i].size() > 0) {
			const element& el = _buckets[i].pop_back();
			const key_t new_bucket = _get_bucket_no(el.key);
			_buckets[new_bucket].push_back(el);
			_buckets_min[new_bucket] = element::min(_buckets_min[new_bucket], el.key);
			// Element przenosimy na koniec nowego kube³ka
			_element_positions[el.value] = _buckets[new_bucket].size() - 1;
		}

		return _remove_least();
	}

	/**
	 * Wstawianie nowej wartoœci do kolejki
	 *
	 * @param item wartoœæ do wstawienia
	 * @param key priorytet elementu do wstawienia
	 */
	void push(const value_t& item, const key_t key) {
		assert(MAX_PRIORITY >= key);

		++_items_count;
		// Numer kube³ka do którego powinien wyl¹dowaæ ten element
		const key_t bucket_no = _get_bucket_no(key);
		const element new_element(item, key);
		_buckets[bucket_no].push_back(new_element);
		_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], key);
		// Aktualizacja lookup table - wstawiamy na ostatniej pozycji
		_element_positions[item] = _buckets[bucket_no].size() - 1;
	}

	/**
	 * Zmiana priorytetu elementu bêd¹cego ju¿ w kolejce
	 *
	 * @param item wartoœæ dla której ma byæ zmieniony priorytet
	 * @param old_key poprzedni priorytet dla elementu
	 * @param new_key nowy priorytet dla elementu
	 */
	void change_priority(const value_t& item, const key_t old_key, const key_t new_key) {
		// Usuwanie elementu z kolejki
		const key_t bucket_no = _get_bucket_no(old_key);
		const position_t item_pos = _element_positions[item];
		// 2 mo¿liwoœci - usuwamy z koñca kube³ka, b¹dŸ z jego œrodka
		const element& last = _buckets[bucket_no].pop_back();
		// Usuwamy ze œrodka - aktualizacja miejsca dla ostatniego elementu
		if (item_pos != _buckets[bucket_no].size() - 1) {
			_buckets[bucket_no][item_pos] = last;
			_element_positions[last.value] = item_pos;
		}
		_element_positions[item] = NON_EXISTING_POS;

		// Przypadek szczególny - zmieniamy priorytet najmniejszego elementu
		// W takim przypadku trzeba odszukaæ element minimalny w tym kube³ku
		_fix_minimum(bucket_no, old_key);

		// Dodanie elementu jeszcze raz
		--_items_count;
		push(item, new_key);
	}

	// Iloœæ elementów w kolejce
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
	 * Wypisywanie zawartoœci poszczególnych kube³ków
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
	// Numer kube³ka jest okreœlany przez najwy¿szy ustawiony 
	// __lzcnt jest nieznacznie szybsze od _BitScanReverse, ale wymaga obs³ugi SSE4
	key_t _get_bucket_no(const key_t key) {
		if (key == _last_deleted) return 0;

		key_t res = __lzcnt(key ^ _last_deleted);
		//unsigned long res;
		//_BitScanReverse(&res, key ^ _last_deleted);
		assert(res + 1 < _buckets_no);
		return res + 1;
	}

	/**
	 * Usuniêcie najmniejszego elementu z kolejki i zwrócenie go
	 * Zwraca najmniejszy element (wczeœniej umieszczony w kube³ku #0)
	 */
	element& _remove_least() {
		// Zwrócenie elementu najmniejszego - zawsze w zerowym kube³ku
		--_items_count;
		element& least = _buckets[0].pop_back();
		// Usuniêcie z lookup table
		_element_positions[least.value] = NON_EXISTING_POS;
		return least;
	}
	/**
	 * Szukanie nowego minimum dla okreœlonego kube³ka
	 * Jeœli usuniêta wartoœæ nie by³a minimum w kube³ku, funkcja nic nie robi
	 *
	 * @param bucket_no numer kube³ka dla którego ma byæ poprawione minimum
	 * @param old_key usuniêta wartoœæ
	 */
	void _fix_minimum(const key_t bucket_no, const key_t old_key) {
		if (_buckets_min[bucket_no] == old_key) {
			_buckets_min[bucket_no] = NON_EXISTING_KEY;
			for (position_t i = 0; i < _buckets[bucket_no].size(); ++i) {
				_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], _buckets[bucket_no][i].key);
			}
		}
	}

	// Maksymalna wartoœæ klucza/priorytetu
	static const key_t MAX_PRIORITY = UINT_MAX;
	// Wartoœæ oznaczaj¹ca coœ nieistniej¹cego
	static const key_t NON_EXISTING_KEY = std::numeric_limits<key_t>::max();
	static const position_t NON_EXISTING_POS = std::numeric_limits<position_t>::max();

	// Kube³ki i dane z nimi zwi¹zane
	key_t _buckets_no;
	// Ostatnio usuniêta wartoœæ
	key_t _last_deleted;
	// Iloœæ elementów w kolejce
	position_t _items_count;
	bucket_t* _buckets = nullptr;
	// Minima dla poszczególnych kube³ków
	key_t* _buckets_min = nullptr;

	// Lookup table dla wyszukiwania w reduce_priority
	position_t* _element_positions = nullptr;
	// Wielkoœæ lookup table
	position_t _element_count;
};