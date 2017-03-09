// radix-heap.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vector.h"
#include "radix.h"
#include <queue>
#include <vector>
#include <list>

int main()
{
	// Problem #1: warto�ci (elementy, nie priorytety) w kolejce musz� by� unikalne je�li u�ywamy lookup table
	// Problem #2: warto�ci nie s� sprawdzane (np. czy spe�niaj� warunki wstawienia), ale to oszcz�dza par� cykli
	// Problem #3: resize-down wektora spowalnia kolejk� o... ~30%-50%

	typedef RadixHeap<unsigned, unsigned> rheap;
	typedef std::pair<unsigned, unsigned> iipair;
	typedef std::vector<iipair> iiv;
//#define _BIG_DEBUG 1
#ifdef _BIG_DEBUG
	rheap::element tmp;

	int count = 1 << 22;
// Duuuuu�a ilo�� danych dla rheap
#if _BIG_DEBUG == 0
	rheap heap(count);
	// Dodawanie
	for (int i = 0; i < count; ++i) {
		heap.push(i, i / 12);
	}
#if USE_LOOKUP_TABLES
	for (int i = 100; i < (count >> 1); ++i) {
		heap.reduce_priority(i, i / 12 - 2);
	}
#endif
// Ma�a (sta�a) ilo�� danych dla kolejki
#elif _BIG_DEBUG == 1
					//0  1  2  3 4  5  6  7  8  9 10 11 12 13 14 15 16 17
	int numbers[] = { 7,58,59,13,8,49,51,23,30,16,39,11,10,9, 63,33,48,57};
	count = sizeof(numbers) / sizeof(numbers[0]);

	rheap heap(count);
	for (int i = 0; i < count; ++i) heap.push(i, numbers[i]);
	heap.dump();
	int numbers2[] = { 2, 0, 4 };
	for (int i = 0; i < 3; ++i) heap.reduce_priority(numbers2[i], numbers[numbers2[i]] - 3);
	heap.dump();
#endif
// Opr�nianie kolejki
#if _BIG_DEBUG < 2
	// Usuwanie
	while (!heap.empty()) {
		heap.pop();
	}
	printf("\n\nKolejka oprozniona");
#endif
// Test szybko�ci dla std::make_heap i std::pop_heap
#if _BIG_DEBUG == 2
	std::vector<unsigned> heap;
	for (int i = 0; i < count; ++i) heap.push_back(i);
	std::make_heap(heap.begin(), heap.end());
	while (!heap.empty()) {
		std::pop_heap(heap.begin(), heap.end());
		heap.pop_back();
	}
#endif
	const unsigned vec_test = 1 << 22;
// zachowanie struktur dla du�ej ilo�ci danych (push/pop back)
#if _BIG_DEBUG == 3 // std::vector
	std::vector<unsigned> test;
#elif _BIG_DEBUG == 4 // custom vector
	vector<unsigned> test;
#elif _BIG_DEBUG == 5 // std::list
	std::list<unsigned> test;
#endif
// Kod niezale�ny od ww. struktur
#if _BIG_DEBUG >= 3 && _BIG_DEBUG <= 5
	for (unsigned i = 0; i < vec_test; ++i) test.push_back(i);
	while (!test.empty()) test.pop_back();
#endif
// Kolejka ma troch� inne API
#if _BIG_DEBUG == 6 // std::queue
	std::queue<unsigned> test;
	for (unsigned i = 0; i < vec_test; ++i) test.push(i);
	while (!test.empty()) test.pop();
#endif
#if _BIG_DEBUG == 7 // std::sort
	std::vector<unsigned> test;
	for (unsigned i = 0; i < vec_test; ++i) test.push_back(i);
	std::sort(test.begin(), test.end());
#endif
#else
	printf("%d", __lzcnt(8 ^ 11) + 1);
#endif
	 
	return 0;
}

