#pragma once

#include "vector.h"
#include <cassert>

/**
 * Czy ma byæ u¿yte cachowanie pozycji i priorytetów?
 * Poch³ania to wiêksz¹ iloœæ pamiêci i mo¿e nie byæ wymagane jeœli nie trzeba modyfikowaæ priorytetu elementów.
 * Mo¿e nieznacznie (~5-10%) spowalniaæ kolejkê (push/pop bez redukcji priorytetu), ale umo¿liwia zmianê
 * priorytetu w czasie mniej-wiêcej sta³ym. Dodatkowo, cachowanie wymusza wrzucanie do kolejki wy³¹cznie
 * unikalnych wartoœci (przez u¿ycie trywialnej funkcji hashuj¹cej)
 */
#ifndef USE_LOOKUP_TABLES
#define USE_LOOKUP_TABLES 1
#endif
 /**
  * Czy u¿ywaæ __lzcnt() wymagaj¹cego obs³ugi SSE4 przez procesor
  * Jeœli 0, bêdzie u¿yte nieznacznie wolniejsze _BitScanReverse()
  * Jeœli CPU nie obs³uguje ABM wyst¹pi undefined behaviour
  */
#ifndef USE_ABM_LZCNT
#define USE_ABM_LZCNT 1
#endif
  /**
   * Czy Szukanie minimum w kube³ku ma siê odbywaæ przy ka¿dej operacji (bêdzie cachowane dla pop()),
   * czy tylko w pop()
   * Przy pewnych danych wy³¹czenie mo¿e przyspieszyæ, m.in. gdy jest wiele pesymistycznych przypadków
   * reduce_priority (przenosimy minimum do innego kube³ka)
   * Wy³¹czenie gdy USE_LOOKUP_TABLES == 0 nie ma sensu, bo oszczêdzi niewiele pamiêci a spowolni pop()
   */
#ifndef CACHE_MIN
#define CACHE_MIN 1
#endif

   /**
	* Radix heap - "kopiec kube³kowy"
	* Dzia³a zak³adaj¹c, ¿e priorytet jest liczb¹ ca³kowit¹ >= 0
	* Pozwala na implementacjê alg. Dijkstry o z³o¿onoœci ~O(m+n*logC)
	* Przy okazji jest znacznie prostszy w implementacji ni¿ np. Kopiec Fibonacciego
	* Wersja wykorzystuj¹ca lookup table (USE_LOOKUP_TABLES = 1) dzia³a tylko jeœli value_t
	* jest liczb¹ ca³kowit¹ ca³kowit¹ (np. numer wierzcho³ka w alg. Dijkstry).
	*
	* Testy jednostkowe, ³¹cznie 20+ z u¿yciem boost::test potwierdzaj¹ poprawnoœæ dzia³ania
	*
	* Ww. dyrektywy u¿ywane s¹ do warunkowej kompilacji kodu dziêki czemu mo¿na go dostosowaæ
	* w zale¿noœci od wymagañ (zu¿ycie pamiêci/szybkoœæ). W³¹czenie obu rodzajów cache mo¿e
	* znacznie przyspieszyæ dzia³anie kodu, ALE powoduje zu¿ycie ~40% wiêcej pamiêci
	*
	* Ta implementacja wykorzystuje vectory zamiast list poniewa¿:
	* - vector zajmuje zauwa¿alnie mniej pamiêci (wielkoœæ jest regulowana automatycznie)
	* - tablica jest przechowywana w sposób ci¹g³y, co znacznie zmniejsza czas dostêpu do danych
	* - u¿ywane operacje dodawania/usuwania elementów wykonuj¹ siê w czasie O(1) - push_back/pop_back
	* - okrojony vector w tej implementacji jest znacznie szybszy od std::vector (push_back/emplace_back)
	* - testy wg. Visuala: http://ideone.com/wxwrwg
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

	// Typ okreœlaj¹cy poszczególne kube³ki
	typedef vector<element> bucket_t;
#if USE_LOOKUP_TABLES
	// Typ przechowuj¹cy miejsce w kube³ku dla poszczcególnych wartoœci
	// Jeœli elementów jest ma³o (<2^8/2^16), mo¿e byæ mniejszy typ - mniej pamiêci
	typedef size_t position_t;
#endif

	/**
	 * Konstruktor
	 * @param element_count maksymalna iloœæ elementów (np. wierzcho³ków) przechowywanych w kopcu
	 */
#if USE_LOOKUP_TABLES
	RadixHeap() : _items_count(0), _element_count(0) {}

	explicit RadixHeap(const position_t element_count) : _element_count(element_count) {
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
		// iloœæ kube³kow ~ iloœæ bitów maksymalnej wielkoœci klucza
		_buckets_no = static_cast<key_t>(ceil(log2(MAX_PRIORITY)) + 1);
		_last_deleted = 0;

		// Alokacja kube³ków
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
	 * Jeœli jest kilka elementów minimalnych o tej samej wartoœci (priorytecie)
	 * to delete_min bêdzie je pakowa³o ci¹gle do kube³ka #0 po usuniêciu pierwszego z nich
	 * wiêc jeœli coœ jest w #0 mo¿na pomin¹æ redystrybucjê
	 *
	 * Zwraca najmniejszy element w kolejce
	 */
	element& pop() {
		assert(_items_count > 0);
		if (!_items_count) return element();
		// Jeœli coœ jest w kube³ku #0 to jest to minimum
		if (!_buckets[0].empty()) return _remove_least();

		// Szukanie pierwszego, niepustego kube³ka
		size_t i = 0;
		for (; _buckets[i].empty(); ++i);
		// Usuniêty jest element o najmniejszym priorytecie z tego kube³ka
#if CACHE_MIN
		_last_deleted = _buckets_min[i];
		_buckets_min[i] = NON_EXISTING_KEY;
#else
		_last_deleted = NON_EXISTING_KEY;
		for (size_t j = 0; j < _buckets[i].size(); ++j)
			_last_deleted = element::min(_buckets[i][j].key, _last_deleted);
#endif

		// Redystrybucja elementów
		while (!_buckets[i].empty()) {
			const element& el = _buckets[i].pop_back();
			const key_t new_bucket = _find_bucket(el.key);
			_buckets[new_bucket].push_back(el);
#if CACHE_MIN
			_buckets_min[new_bucket] = element::min(_buckets_min[new_bucket], el.key);
#endif
#if USE_LOOKUP_TABLES
			// Element przenosimy na koniec nowego kube³ka
			_element_positions[el.value] = _buckets[new_bucket].size() - 1;
#endif
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
	 * Zmiana priorytetu elementu bêd¹cego ju¿ w kolejce
	 *
	 * @param item wartoœæ dla której ma byæ zmieniony priorytet
	 * @param new_key nowy priorytet dla elementu
	 */
	void reduce_priority(const value_t& item, const key_t new_key) {
		// Usuwanie elementu z kolejki
		const key_t old_key = _current_priorities[item];
		const key_t bucket_no = _find_bucket(old_key);
		const position_t item_pos = _element_positions[item];
		assert(new_key <= old_key);
		// Jeœli element pozostaje w tym kube³ku, to nie trzeba go przesuwaæ
		if (bucket_no == _find_bucket(new_key)) {
			_current_priorities[item] = new_key;
			_buckets[bucket_no][item_pos].key = new_key;
#if CACHE_MIN
			_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], new_key);
#endif
			return;
		}
		// 2 mo¿liwoœci - usuwamy z koñca kube³ka, b¹dŸ z jego œrodka
		const element& last = _buckets[bucket_no].pop_back();
		// Usuwamy ze œrodka - aktualizacja miejsca dla ostatniego elementu
		if (item_pos != _buckets[bucket_no].size()) {
			_buckets[bucket_no][item_pos] = last;
			_element_positions[last.value] = item_pos;
		}

#if CACHE_MIN
		// Przypadek szczególny - zmieniamy priorytet najmniejszego elementu
		// W takim przypadku trzeba odszukaæ element minimalny w tym kube³ku
		_fix_minimum(bucket_no, old_key, new_key);
#endif

		// Dodanie elementu jeszcze raz
		--_items_count;
		push(item, new_key);
	}
#endif

	// Iloœæ elementów w kolejce
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
	 * Wypisywanie zawartoœci poszczególnych kube³ków
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
	 * Numer kube³ka jest okreœlany przez najwy¿szy ustawiony bit
	 * @param key klucz (priorytet) dla szukanego elementu
	 */
	key_t _find_bucket(const key_t key) const {
		if (key == _last_deleted) return 0;
#if USE_ABM_LZCNT
		key_t res = __lzcnt(key ^ _last_deleted);
#else
		unsigned long res;
		_BitScanReverse(&res, key ^ _last_deleted);
#endif
		assert(res + 1 < _buckets_no);
		return res + 1;
	}

	/**
	 * Usuniêcie najmniejszego elementu z kolejki i zwrócenie go
	 * Zwraca najmniejszy element (wczeœniej umieszczony w kube³ku #0)
	 */
	element& _remove_least() {
		assert(_items_count > 0);
		// Zwrócenie elementu najmniejszego - zawsze w zerowym kube³ku
		--_items_count;
		element& least = _buckets[0].pop_back();
#if USE_LOOKUP_TABLES
		// Usuniêcie z lookup table
		_element_positions[least.value] = NON_EXISTING_POS;
		_current_priorities[least.value] = NON_EXISTING_KEY;
#endif
		return least;
	}

#if CACHE_MIN && USE_LOOKUP_TABLES
	/**
	 * Szukanie nowego minimum dla okreœlonego kube³ka
	 * Jeœli usuniêta wartoœæ nie by³a minimum w kube³ku, funkcja nic nie robi
	 * Jeœli po zmianie priorytetu wartoœæ zostaje w kube³ku, sprawdzanie jest pominiête
	 *
	 * @param bucket_no numer kube³ka dla którego ma byæ poprawione minimum
	 * @param old_key usuniêta wartoœæ
	 * @param old_key nowy priorytet dla wartoœci
	 */
	void _fix_minimum(const key_t bucket_no, const key_t old_key, const key_t new_key) {
		if (_buckets_min[bucket_no] != old_key || _find_bucket(old_key) == _find_bucket(new_key))
			return;
		// Poprzednie minimum dla tego kube³ka
		const key_t old_min = _buckets_min[bucket_no];
		_buckets_min[bucket_no] = NON_EXISTING_KEY;
		for (position_t i = 0; i < _buckets[bucket_no].size(); ++i) {
			// Mamy kilka elementów minimalnych
			if (old_min == _buckets[bucket_no][i].key) {
				_buckets_min[bucket_no] = old_min;
				return;
			}
			_buckets_min[bucket_no] = element::min(_buckets_min[bucket_no], _buckets[bucket_no][i].key);
		}
	}
#endif

	// Maksymalna wartoœæ klucza/priorytetu
	static const key_t MAX_PRIORITY = UINT_MAX;
	// Wartoœæ oznaczaj¹ca nieistniej¹cy klucz (priorytet)
	static const key_t NON_EXISTING_KEY = std::numeric_limits<key_t>::max();
#if USE_LOOKUP_TABLES
	// Wartoœæ oznaczaj¹ca nieistniej¹c¹ pozycjê (nie ma w kolejce)
	static const position_t NON_EXISTING_POS = std::numeric_limits<position_t>::max();
#endif

	// Kube³ki i dane z nimi zwi¹zane
	key_t _buckets_no;
	// Ostatnio usuniêta wartoœæ
	key_t _last_deleted;
	// Iloœæ elementów w kolejce
	size_t _items_count;
	bucket_t* _buckets = nullptr;
#if CACHE_MIN
	// Minima dla poszczególnych kube³ków
	key_t* _buckets_min = nullptr;
#endif

#if USE_LOOKUP_TABLES
	// Lookup table dla wyszukiwania w reduce_priority
	position_t* _element_positions = nullptr;
	// Wielkoœæ lookup table
	position_t _element_count;

	// Cachowane priorytety dla poszczególnych wartoœci
	key_t* _current_priorities;
#endif
};
