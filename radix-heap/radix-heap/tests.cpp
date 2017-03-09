#define BOOST_TEST_MODULE radix heap test suite
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <boost/range/irange.hpp>
#include <vector>
#include "radix.h"

typedef RadixHeap<unsigned, unsigned> rheap;
typedef std::pair<unsigned, unsigned> iipair;
typedef std::vector<iipair> iiv;
typedef std::vector<int> iv;

/**
 * Testy z podstawowymi danymi
 */
struct basic_fixture {
	// Set up
	basic_fixture() {
		BOOST_TEST_MESSAGE("set up basic_fixture...");

		// Liczby do wrzucenia do kolejki
		unsigned x = 0;
				   // x :  0   1   2   3  4   5   6   7   8   9  10  11  12  13  14  15  16  17
		for (unsigned i : {7, 58, 59, 13, 8, 49, 51, 23, 30, 16, 39, 11, 10,  9, 63, 33, 48, 57})
			n.push_back(iipair(x++, i));

		// priorytety: 7,8,9,10,11,13,16,23,30,33,39,48,49,51,57,58,59,63
		// posortowane wg. prio: v : 0 4 13 12 11  3  9  7  8 15 10 16  5  6 17  1  2 14
		//						 k : 7 8 9  10 11 13 16 23 30 33 39 48 49 51 57 58 59 63
		/* pocz¹tkowo w kube³kach (v,k): */
		// 0 : -
		// 1 : -
		// 2 : -
		// 3 : ( 0, 7)
		// 4 : ( 3,13) ( 4, 8) (11,11) (12,10) (13, 9)
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57) (14,63)
		/* dystrybucja po 2. POPie (last-deleted: 0 -> 7 -> 8): */
		// 0 : -
		// 1 : (13, 9)
		// 2 : (11,11) (12,10)
		// 3 : ( 3,13) 
		// 4 : -
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57) (14,63)

		// Wrzucanie elementów do kolejki
		heap = new rheap(0xFF);
		std::for_each(n.begin(), n.end(), [&](auto i) { heap->push(i.first, i.second); });
	}

	// Usuwanie wszystkich elementów z kolejki i wyci¹gniêcie kluczy
	void _pop_keys_from_heap(int amount = 0) {
		int i = 0;
		while (!amount ? !heap->empty() : i++ < amount) {
			auto tmp = heap->pop();
			keys_res.push_back(tmp.key);
		}
	}

	// Tear down
	~basic_fixture() {
		BOOST_TEST_MESSAGE("tear down basic_fixture...");
		delete heap;
	}

	// Elementy przechowywane jako para [wartoœæ, klucz]
	iiv n; // ród³owe dane
	rheap *heap;
	iv keys_res; // Rezultat z _pop_keys_from_heap()
};
BOOST_AUTO_TEST_SUITE(Basic_Data)

BOOST_FIXTURE_TEST_CASE(Push_Elements, basic_fixture)
{
	BOOST_CHECK_EQUAL(heap->size(), n.size());
}

BOOST_FIXTURE_TEST_CASE(Pop_Elements, basic_fixture)
{
	// Operacje
	_pop_keys_from_heap(n.size());

	// Kolejka powinna byæ pusta
	BOOST_CHECK_EQUAL(heap->size(), 0);
	// Sprawdzenie czy kolejnoœæ jest ok
	iv expected = { 7,8,9,10,11,13,16,23,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

BOOST_FIXTURE_TEST_CASE(Reduce_Priority, basic_fixture)
{
	heap->reduce_priority(13, 5);
	BOOST_CHECK_EQUAL(heap->size(), n.size());
	auto tmp = heap->pop();
	BOOST_CHECK_EQUAL(tmp.value, 13);
	BOOST_CHECK_EQUAL(tmp.key, 5);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 7,8,10,11,13,16,23,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu dla minimum z kube³ka (przenosz¹c do innego)
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_For_Bucket_Min, basic_fixture)
{
	// Przenosimy element o wartoœci 4 na pocz¹tek kolejki
	heap->reduce_priority(4, 6);
	BOOST_CHECK_EQUAL(heap->size(), n.size());
	auto tmp = heap->pop();
	BOOST_CHECK_EQUAL(tmp.value, 4);
	BOOST_CHECK_EQUAL(tmp.key, 6);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 7,9,10,11,13,16,23,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu pozostawiaj¹c w tym samym kube³ku
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Same_Bucket, basic_fixture)
{
	heap->reduce_priority(8, 25);
	BOOST_CHECK_EQUAL(heap->size(), n.size());

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 7,8,9,10,11,13,16,23,25,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu pozostawiaj¹c w tym samym kube³ku (minimum z kube³ka)
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Min_Same_Bucket, basic_fixture)
{
	heap->reduce_priority(15, 32);
	BOOST_CHECK_EQUAL(heap->size(), n.size());

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 7,8,9,10,11,13,16,23,30,32,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu po zdjêciu czegoœ
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, ¿eby mieæ jakiœ redystrybuowany kube³ek
	heap->reduce_priority(7, 20);
	BOOST_REQUIRE_EQUAL(heap->size(), n.size() - 2);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 9,10,11,13,16,20,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu dla minimum z kube³ka (przenosz¹c do innego) po zdjêciu czegoœ
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_For_Min_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, ¿eby mieæ jakiœ redystrybuowany kube³ek
	heap->reduce_priority(12, 9);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 9,9,11,13,16,23,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu pozostawiaj¹c w tym samym kube³ku po zdjêciu czegoœ
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Same_Bucket_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, ¿eby mieæ jakiœ redystrybuowany kube³ek
	heap->reduce_priority(17, 50);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 9,10,11,13,16,23,30,33,39,48,49,50,51,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu pozostawiaj¹c w tym samym kube³ku (minimum z kube³ka) po zdjêciu czegoœ
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Min_Same_Bucket_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, ¿eby mieæ jakiœ redystrybuowany kube³ek
	heap->reduce_priority(15, 32);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 9,10,11,13,16,23,30,32,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Dodanie elementu po zdjêciu czegoœ
BOOST_FIXTURE_TEST_CASE(Push_After_Pop, basic_fixture)
{
	const unsigned NEW_VAL = 0xF0, NEW_KEY = 8;
	// Zdejmujemy najmniejszy element [0,7]
	heap->pop();
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 1);

	// Dodajemy nowy element
	heap->push(NEW_VAL, NEW_KEY);
	BOOST_CHECK_EQUAL(heap->size(), n.size());

	// Zdejmujemy najmniejszy element
	auto tmp = heap->pop();
	// Na szczycie powinien byæ nadal element który by³ #2
	BOOST_CHECK_EQUAL(tmp.value, 4);
	BOOST_CHECK_EQUAL(tmp.key , 8);

	// Kolejny element -> powinien byæ ten przed chwil¹ wrzucony
	tmp = heap->pop();
	BOOST_CHECK_EQUAL(tmp.value, NEW_VAL);
	BOOST_CHECK_EQUAL(tmp.key, NEW_KEY);

	// Dodaliœmy 1 element, 3 usunêliœmy
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);
	// @todo: reszta kolejki
}

// Zmiana priorytetu dla ostatniego elementu w kube³ku
BOOST_FIXTURE_TEST_CASE(Change_Prio_Last_From_Bucket, basic_fixture)
{
	heap->reduce_priority(13,8);

	// Sprawdzenie kolejki
	_pop_keys_from_heap();
	iv expected = { 7,8,8,10,11,13,16,23,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Zmiana priorytetu dla ostatniego elementu w kube³ku po zdjêciu czegoœ
BOOST_FIXTURE_TEST_CASE(Change_Prio_Last_Bucket_After_Pop, basic_fixture)
{
	heap->pop();
	heap->reduce_priority(13, 8);

	// Sprawdzenie kolejki
	_pop_keys_from_heap();
	iv expected = { 8,8,10,11,13,16,23,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Zmiana priorytetu dla ostatniego elementu w kube³ku po redystrybucji
BOOST_FIXTURE_TEST_CASE(Change_Prio_Last_Bucket_After_Redist, basic_fixture)
{
	heap->pop();
	heap->pop();
	heap->reduce_priority(12, 9);

	// Sprawdzenie kolejki
	_pop_keys_from_heap();
	iv expected = { 9,9,11,13,16,23,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// priorytety: 7,8,9,10,11,13,16,23,30,33,39,48,49,51,57,58,59,63
// Zmiana priorytetu dla przedostatniego elementu w kube³ku
BOOST_FIXTURE_TEST_CASE(Change_Prio_Last1_From_Bucket, basic_fixture)
{
	heap->reduce_priority(8, 15);

	// Sprawdzenie kolejki
	_pop_keys_from_heap();
	iv expected = { 7,8,9,10,11,13,15,16,23,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Zmiana priorytetu dla przedostatniego elementu w kube³ku po zdjêciu czegoœ
BOOST_FIXTURE_TEST_CASE(Change_Last1_Bucket_After_Pop, basic_fixture)
{
	heap->pop();
	heap->reduce_priority(8, 15);

	// Sprawdzenie kolejki
	_pop_keys_from_heap();
	iv expected = { 8,9,10,11,13,15,16,23,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Zmiana priorytetu dla przedostatniego elementu w kube³ku po redystrybucji
BOOST_FIXTURE_TEST_CASE(Change_Last1_Bucket_After_Redist, basic_fixture)
{
	heap->pop();
	heap->pop();
	heap->reduce_priority(8, 20);

	// Sprawdzenie kolejki
	_pop_keys_from_heap();
	iv expected = { 9,10,11,13,16,20,23,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
///////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(Big_Data)

/**
 * Operacje na du¿ej iloœci danych
 */

BOOST_AUTO_TEST_CASE(Push_100k)
{
	const int COUNT = 100 * 1000;
	rheap heap(COUNT);
	for (size_t i : boost::irange(0, COUNT)) heap.push(i, i);
	BOOST_CHECK_EQUAL(heap.size(), COUNT);
}

BOOST_AUTO_TEST_CASE(Push_Pop_100k)
{
	iv src, res;
	const int COUNT = 100 * 1000;
	// Push
	rheap heap(COUNT);
	for (size_t i : boost::irange(0, COUNT)) {
		heap.push(i, i);
		src.emplace_back(i);
	}
	BOOST_CHECK_EQUAL(heap.size(), COUNT);

	// Pop
	while (!heap.empty()) res.emplace_back(heap.pop().value);
	BOOST_TEST(src == res);
}

/**
 * Wrzucanie elementów w kolejnoœci malej¹cej
 */
BOOST_AUTO_TEST_CASE(Push_50k_reverse)
{
	const int COUNT = 50 * 1000;
	rheap heap(COUNT);
	for (size_t i : boost::irange(0, COUNT)) heap.push(i, COUNT - i);
	BOOST_CHECK_EQUAL(heap.size(), COUNT);
}

BOOST_AUTO_TEST_CASE(Push_Pop_50k_reverse)
{
	iv src, res;
	const int COUNT = 50*1000;
	// Push
	rheap heap(COUNT);
	for (size_t i : boost::irange(0, COUNT)) {
		heap.push(i, COUNT - i);
		// priorytety bêd¹ ros³y odwrotnie do wartoœci i
		src.emplace_back(COUNT - i - 1);
	}
	BOOST_CHECK_EQUAL(heap.size(), COUNT);

	// Pop
	while (!heap.empty()) res.emplace_back(heap.pop().value);
	BOOST_TEST(src == res);
}

BOOST_AUTO_TEST_SUITE_END()
///////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(Same_Priorities)

/**
 * Testy dla x elementów o tym samym priorytecie
 */
struct recurring_fixture {
	// Set up
	recurring_fixture() {
		BOOST_TEST_MESSAGE("set up recurring_fixture...");

		// Liczby do wrzucenia do kolejki
		unsigned x = 0;
		//			  x :  0  1   2   3  4   5   6   7   8   9  10  11  12  13  14 
		for (unsigned i : {7, 7, 13, 13, 8, 13, 11, 16, 30, 16, 39, 39, 39, 39, 63})
			n.push_back(iipair(x++, i));

		// priorytety: 7,7,8,11,13,13,13,16,16,30,39,39,39,39,63
		// posortowane wg. prio: v : 0 1 4  6  2  3  5  7  9  8 10 11 12 13 14
		//						 k : 7 7 8 11 13 13 13 16 16 30 39 39 39 39 63
		/* pocz¹tkowo w kube³kach (v,k): */
		// 0 : -
		// 1 : -
		// 2 : -
		// 3 : ( 0, 7) ( 1, 7)
		// 4 : ( 2,13) ( 3,13) ( 4,8) ( 5,13) ( 6,11)
		// 5 : ( 7,16) ( 8,30) ( 9,16)
		// 6 : (10,39) (11,39) (12,39) (13,39) (14,63)
		/* dystrybucja po 3. POPie (last-deleted: 0 -> 7 -> 7 -> 8): */
		// 0 : -
		// 1 : -
		// 2 : ( 6,11)
		// 3 : ( 5,13) ( 3,13), ( 2,13)
		// 4 : -
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57) (14,63)

		// Wrzucanie elementów do kolejki
		heap = new rheap(0xFF);
		std::for_each(n.begin(), n.end(), [&](auto i) { heap->push(i.first, i.second); });
	}

	// Usuwanie wszystkich elementów z kolejki i wyci¹gniêcie kluczy
	void _pop_keys_from_heap(int amount = 0) {
		int i = 0;
		while (!amount ? !heap->empty() : i++ < amount) {
			auto tmp = heap->pop();
			keys_res.push_back(tmp.key);
		}
	}
	// zdejmowanie z kolejki ¿eby by³a redystrybucja
	void pop_to_redistribute() {
		for (int i = 0; i < 3; ++i) heap->pop();
	}

	// Tear down
	~recurring_fixture() {
		BOOST_TEST_MESSAGE("tear down recurring_fixture...");
		delete heap;
	}

	// Elementy przechowywane jako para [wartoœæ, klucz]
	iiv n; // ród³owe dane
	rheap *heap;
	iv keys_res; // Rezultat z _pop_keys_from_heap()
};

BOOST_FIXTURE_TEST_CASE(Push_Elements, recurring_fixture)
{
	BOOST_CHECK_EQUAL(heap->size(), n.size());
}

BOOST_FIXTURE_TEST_CASE(Pop_Elements, recurring_fixture)
{
	// Operacje
	_pop_keys_from_heap(n.size());

	// Kolejka powinna byæ pusta
	BOOST_CHECK_EQUAL(heap->size(), 0);
	// Sprawdzenie czy kolejnoœæ jest ok
	iv expected = { 7,7,8,11,13,13,13,16,16,30,39,39,39,39,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

BOOST_FIXTURE_TEST_CASE(Reduce_Priority, recurring_fixture)
{
	heap->reduce_priority(8, 15);
	BOOST_CHECK_EQUAL(heap->size(), n.size());

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 7,7,8,11,13,13,13,15,16,16,39,39,39,39,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}


BOOST_AUTO_TEST_SUITE_END()
