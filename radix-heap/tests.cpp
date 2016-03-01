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

// @todo: zmiana priorytetu dla elementu z ostatniego/przedostatniego miejsca w kube≥ku do innego kube≥ka

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
		/* poczπtkowo w kube≥kach (v,k): */
		// 0 : -
		// 1 : -
		// 2 : -
		// 3 : ( 0, 7)
		// 4 : ( 3,13) ( 4, 8) (11,11) (12,10) (13, 9)
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57) (14,63)
		/* dystrybucja po 2. POPie (last-deleted: 0 ? 7 ? 8): */
		// 0 : -
		// 1 : (13, 9)
		// 2 : (11,11) (12,10)
		// 3 : ( 3,13) 
		// 4 : -
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57) (14,63)

		// Wrzucanie elementÛw do kolejki
		heap = new rheap(0xFF);
		std::for_each(n.begin(), n.end(), [&](auto i) { heap->push(i.first, i.second); });
	}

	// Usuwanie wszystkich elementÛw z kolejki i wyciπgniÍcie kluczy
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

	// Elementy przechowywane jako para [wartoúÊ, klucz]
	iiv n; // èrÛd≥owe dane
	rheap *heap;
	iv keys_res; // Rezultat z _pop_keys_from_heap()
};

BOOST_FIXTURE_TEST_CASE(Push_Elements, basic_fixture)
{
	BOOST_CHECK_EQUAL(heap->size(), n.size());
}

BOOST_FIXTURE_TEST_CASE(Pop_Elements, basic_fixture)
{
	// Operacje
	_pop_keys_from_heap(n.size());

	// Kolejka powinna byÊ pusta
	BOOST_CHECK_EQUAL(heap->size(), 0);
	// Sprawdzenie czy kolejnoúÊ jest ok
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

// Redukcja priorytetu dla minimum z kube≥ka (przenoszπc do innego)
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_For_Bucket_Min, basic_fixture)
{
	// Przenosimy element o wartoúci 4 na poczπtek kolejki
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

// Redukcja priorytetu pozostawiajπc w tym samym kube≥ku
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Same_Bucket, basic_fixture)
{
	heap->reduce_priority(8, 25);
	BOOST_CHECK_EQUAL(heap->size(), n.size());

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 7,8,9,10,11,13,16,23,25,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu pozostawiajπc w tym samym kube≥ku (minimum z kube≥ka)
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Min_Same_Bucket, basic_fixture)
{
	heap->reduce_priority(15, 32);
	BOOST_CHECK_EQUAL(heap->size(), n.size());

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 7,8,9,10,11,13,16,23,30,32,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu po zdjÍciu czegoú
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, øeby mieÊ jakiú redystrybuowany kube≥ek
	heap->reduce_priority(7, 20);
	BOOST_REQUIRE_EQUAL(heap->size(), n.size() - 2);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 9,10,11,13,16,20,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu dla minimum z kube≥ka (przenoszπc do innego) po zdjÍciu czegoú
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_For_Min_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, øeby mieÊ jakiú redystrybuowany kube≥ek
	heap->reduce_priority(12, 9);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	// priorytety: 7,8,9,10,11,13,16,23,30,33,39,48,49,51,57,58,59,63
	// posortowane wg. prio: v : 0 4 13 12 11  3  9  7  8 15 10 16  5  6 17  1  2 14
	//						 k : 7 8 9  10 11 13 16 23 30 33 39 48 49 51 57 58 59 63
	iv expected = { 9,9,11,13,16,23,30,33,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
	// @todo: czemu, do cholery, ten test nie przechodzi?!
}

// Redukcja priorytetu pozostawiajπc w tym samym kube≥ku po zdjÍciu czegoú
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Same_Bucket_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, øeby mieÊ jakiú redystrybuowany kube≥ek
	heap->reduce_priority(17, 50);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 9,10,11,13,16,23,30,33,39,48,49,50,51,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Redukcja priorytetu pozostawiajπc w tym samym kube≥ku (minimum z kube≥ka) po zdjÍciu czegoú
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Min_Same_Bucket_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, øeby mieÊ jakiú redystrybuowany kube≥ek
	heap->reduce_priority(15, 32);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);

	// Sprawdzenie reszty kolejki
	_pop_keys_from_heap();
	iv expected = { 9,10,11,13,16,23,30,32,39,48,49,51,57,58,59,63 };
	BOOST_TEST(expected == keys_res, boost::test_tools::per_element());
}

// Dodanie elementu po zdjÍciu czegoú
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
	// Na szczycie powinien byÊ nadal element ktÛry by≥ #2
	BOOST_CHECK_EQUAL(tmp.value, 4);
	BOOST_CHECK_EQUAL(tmp.key , 8);

	// Kolejny element -> powinien byÊ ten przed chwilπ wrzucony
	tmp = heap->pop();
	BOOST_CHECK_EQUAL(tmp.value, NEW_VAL);
	BOOST_CHECK_EQUAL(tmp.key, NEW_KEY);

	// Dodaliúmy 1 element, 3 usunÍliúmy
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);
}

/**
 * Operacje na duøej iloúci danych
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
 * Wrzucanie elementÛw w kolejnoúci malejπcej
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
		// priorytety bÍdπ ros≥y odwrotnie do wartoúci i
		src.emplace_back(COUNT - i - 1);
	}
	BOOST_CHECK_EQUAL(heap.size(), COUNT);

	// Pop
	while (!heap.empty()) res.emplace_back(heap.pop().value);
	BOOST_TEST(src == res);
}

/**
 * Testy dla x elementÛw o tym samym priorytecie
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

		// priorytety: 7,7,13,13,8,13,11,16,30,16,39,39,39,39,63
		// posortowane wg. prio: v : 0 1 4  6  2  3  5  7  9  8 10 11 12 13 14
		//						 k : 7 7 8 11 13 13 13 16 16 30 39 39 39 39 63
		/* poczπtkowo w kube≥kach (v,k): */ // @todo
		// 0 : -
		// 1 : -
		// 2 : -
		// 3 : ( 0, 7)
		// 4 : ( 3,13) ( 4, 8) (11,11) (12,10) (13, 9)
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57) (14,63)
		/* dystrybucja po 2. POPie (last-deleted: 0 ? 7 ? 8): */
		// 0 : -
		// 1 : (13, 9)
		// 2 : (11,11) (12,10)
		// 3 : ( 3,13) 
		// 4 : -
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57) (14,63)

		// Wrzucanie elementÛw do kolejki
		heap = new rheap(0xFF);
		std::for_each(n.begin(), n.end(), [&](auto i) { heap->push(i.first, i.second); });
	}

	// Usuwanie wszystkich elementÛw z kolejki i wyciπgniÍcie kluczy
	void _pop_keys_from_heap(int amount = 0) {
		int i = 0;
		while (!amount ? !heap->empty() : i++ < amount) {
			auto tmp = heap->pop();
			keys_res.push_back(tmp.key);
		}
	}

	// Tear down
	~recurring_fixture() {
		BOOST_TEST_MESSAGE("tear down recurring_fixture...");
		delete heap;
	}

	// Elementy przechowywane jako para [wartoúÊ, klucz]
	iiv n; // èrÛd≥owe dane
	rheap *heap;
	iv keys_res; // Rezultat z _pop_keys_from_heap()
};

/*
 * Coú dla in_heap()
 */