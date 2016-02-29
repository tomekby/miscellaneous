#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <boost/range/irange.hpp>
#include <vector>
#include "radix.h"

typedef RadixHeap<unsigned, unsigned> rheap;
typedef std::pair<unsigned, unsigned> iipair;
typedef std::vector<iipair> iiv;

// Sprawdzenie, czy elementy s� w kolejno�ci rosn�cej
#define CHECK_ORDER() for (size_t i = 1; i < res.size(); ++i) { \
	BOOST_TEST_CONTEXT("elementy " << res[i - 1].second << " " << res[i].second << " " << i) \
	BOOST_TEST(res[i - 1].second <= res[i].second);\
}

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
		// posortowane wg. prio: v : 0 4 13 12 11  3  9  7  8 15 10 16  5  6 17  1  2 14
		//						 k : 7 8 9  10 11 13 16 23 30 33 39 48 49 51 57 58 59 63
		/* pocz�tkowo w kube�kach (v,k): */
		// 0 : -
		// 1 : -
		// 2 : -
		// 3 : ( 0, 7)
		// 4 : ( 3,13) ( 4, 8) (11,11) (12,10) (13, 9)
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57)
		// 7 : (14,63)
		/* dystrybucja po 2. POPie (last-deleted: 0 ? 7 ? 8): */
		// 0 : -
		// 1 : (13, 9)
		// 2 : (11,11) (12,10)
		// 3 : ( 3,13) 
		// 4 : -
		// 5 : ( 7,23) ( 8,30) ( 9,16)
		// 6 : ( 1,58) ( 2,59) ( 5,49) ( 6,51) (10,39) (15,33) (16,48) (17,57)
		// 7 : (14,63)

		// Wrzucanie element�w do kolejki
		heap = new rheap(0xFF);
		std::for_each(n.begin(), n.end(), [&](auto i) { heap->push(i.first, i.second); });
	}

	// Redukcja kolejki
	void _pop_all_from_heap() {
		while(!heap->empty()) {
			auto tmp = heap->pop();
			res.emplace_back(tmp.value, tmp.key);
		}
	}
	// Tear down
	~basic_fixture() {
		BOOST_TEST_MESSAGE("tear down basic_fixture...");
		delete heap;
	}

	// Elementy przechowywane jako para [warto��, klucz]
	iiv n;
	rheap *heap;
	iiv res;
};

BOOST_FIXTURE_TEST_CASE(Push_Elements, basic_fixture)
{
	BOOST_CHECK_EQUAL(heap->size(), n.size());
}

BOOST_FIXTURE_TEST_CASE(Pop_Elements, basic_fixture)
{
	// Operacje
	_pop_all_from_heap();

	// Kolejka powinna by� pusta
	BOOST_CHECK_EQUAL(heap->size(), 0);
	// Ilo�� element�w w wyniku i �r�dle powinna si� zgadza�
	BOOST_CHECK_EQUAL(res.size(), n.size());
	// Sprawdzenie czy kolejno�� jest ok (rosn�ca)
	CHECK_ORDER()
}

BOOST_FIXTURE_TEST_CASE(Reduce_Priority, basic_fixture)
{
	heap->reduce_priority(13, 5);
	BOOST_CHECK_EQUAL(heap->size(), n.size());
	auto tmp = heap->pop();
	BOOST_CHECK_EQUAL(tmp.value, 13);
	BOOST_CHECK_EQUAL(tmp.key, 5);
}

// Redukcja priorytetu dla minimum z kube�ka (przenosz�c do innego)
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_For_Bucket_Min, basic_fixture)
{
	// Przenosimy element o warto�ci 4 na pocz�tek kolejki
	heap->reduce_priority(4, 6);
	BOOST_CHECK_EQUAL(heap->size(), n.size());
	auto tmp = heap->pop();
	BOOST_CHECK_EQUAL(tmp.value, 4);
	BOOST_CHECK_EQUAL(tmp.key, 6);
}

// Redukcja priorytetu pozostawiaj�c w tym samym kube�ku
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Same_Bucket, basic_fixture)
{
	heap->reduce_priority(8, 25);
	BOOST_CHECK_EQUAL(heap->size(), n.size());
	_pop_all_from_heap();
	CHECK_ORDER()
}

// Redukcja priorytetu pozostawiaj�c w tym samym kube�ku (minimum z kube�ka)
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Min_Same_Bucket, basic_fixture)
{
	heap->reduce_priority(15, 32);
	BOOST_CHECK_EQUAL(heap->size(), n.size());
	_pop_all_from_heap();
	CHECK_ORDER()
}

// Redukcja priorytetu po zdj�ciu czego�
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, �eby mie� jaki� redystrybuowany kube�ek
	heap->reduce_priority(7, 20);
	BOOST_REQUIRE_EQUAL(heap->size(), n.size() - 2);
	_pop_all_from_heap();
	CHECK_ORDER()
}

// Redukcja priorytetu dla minimum z kube�ka (przenosz�c do innego) po zdj�ciu czego�
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_For_Min_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, �eby mie� jaki� redystrybuowany kube�ek
	heap->reduce_priority(12, 9);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);
	_pop_all_from_heap();
	CHECK_ORDER()
}

// Redukcja priorytetu pozostawiaj�c w tym samym kube�ku po zdj�ciu czego�
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Same_Bucket_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, �eby mie� jaki� redystrybuowany kube�ek
	heap->reduce_priority(17, 50);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);
	_pop_all_from_heap();
	CHECK_ORDER()
}

// Redukcja priorytetu pozostawiaj�c w tym samym kube�ku (minimum z kube�ka) po zdj�ciu czego�
BOOST_FIXTURE_TEST_CASE(Reduce_Prio_Min_Same_Bucket_After_Pop, basic_fixture)
{
	heap->pop();
	heap->pop(); // Drugi pop, �eby mie� jaki� redystrybuowany kube�ek
	heap->reduce_priority(15, 32);
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);
	_pop_all_from_heap();
	CHECK_ORDER()
}

// Dodanie elementu po zdj�ciu czego�
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
	// Na szczycie powinien by� nadal element kt�ry by� #2
	BOOST_CHECK_EQUAL(tmp.value, 4);
	BOOST_CHECK_EQUAL(tmp.key , 8);

	// Kolejny element -> powinien by� ten przed chwil� wrzucony
	tmp = heap->pop();
	BOOST_CHECK_EQUAL(tmp.value, NEW_VAL);
	BOOST_CHECK_EQUAL(tmp.key, NEW_KEY);

	// Dodali�my 1 element, 3 usun�li�my
	BOOST_CHECK_EQUAL(heap->size(), n.size() - 2);
}

/**
 * Operacje na du�ej ilo�ci danych
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
	std::vector<unsigned> src, res;
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
 * Wrzucanie element�w w kolejno�ci malej�cej
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
	std::vector<unsigned> src, res;
	const int COUNT = 50*1000;
	// Push
	rheap heap(COUNT);
	for (size_t i : boost::irange(0, COUNT)) {
		heap.push(i, COUNT - i);
		// priorytety b�d� ros�y odwrotnie do i
		src.emplace_back(COUNT - i - 1);
	}
	BOOST_CHECK_EQUAL(heap.size(), COUNT);

	// Pop
	while (!heap.empty()) res.emplace_back(heap.pop().value);
	BOOST_TEST(src == res);
}

/**
 * Testy dla x element�w o tym samym priorytecie
 */
/*
 * Co� dla in_heap()
 */