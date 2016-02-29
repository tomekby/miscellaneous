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
	// Problem #1: wartoœci (elementy, nie priorytety) w kolejce musz¹ byæ unikalne jeœli u¿ywamy lookup table
	// Problem #2: wartoœci nie s¹ sprawdzane (np. czy spe³niaj¹ warunki wstawienia), ale to oszczêdza parê cykli
	// Problem #3: resize-down wektora spowalnia kolejkê o... ~30%-50%

	typedef RadixHeap<unsigned, unsigned> rheap;
	typedef std::pair<unsigned, unsigned> iipair;
	typedef std::vector<iipair> iiv;
//#define _BIG_DEBUG 1
#ifdef _BIG_DEBUG
	rheap::element tmp;

	int count = 1 << 22;
// Duuuuu¿a iloœæ danych dla rheap
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
// Ma³a (sta³a) iloœæ danych dla kolejki
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
// Opró¿nianie kolejki
#if _BIG_DEBUG < 2
	// Usuwanie
	while (!heap.empty()) {
		heap.pop();
	}
	printf("\n\nKolejka oprozniona");
#endif
// Test szybkoœci dla std::make_heap i std::pop_heap
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
// zachowanie struktur dla du¿ej iloœci danych (push/pop back)
#if _BIG_DEBUG == 3 // std::vector
	std::vector<unsigned> test;
#elif _BIG_DEBUG == 4 // custom vector
	vector<unsigned> test;
#elif _BIG_DEBUG == 5 // std::list
	std::list<unsigned> test;
#endif
// Kod niezale¿ny od ww. struktur
#if _BIG_DEBUG >= 3 && _BIG_DEBUG <= 5
	for (unsigned i = 0; i < vec_test; ++i) test.push_back(i);
	while (!test.empty()) test.pop_back();
#endif
// Kolejka ma trochê inne API
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
	iiv n;
	int x = 0;
			   // x :  0   1   2   3  4   5   6   7   8   9  10  11  12  13 14  15  16  17
	for (unsigned i : {7, 58, 59, 13, 8, 49, 51, 23, 30, 16, 39, 11, 10, 9, 63, 33, 48, 57})
		n.push_back(iipair(x++, i));
	rheap heap(0xff);
	std::for_each(n.begin(), n.end(), [&](auto i) { heap.push(i.first, i.second); });
	heap.pop();
	heap.reduce_priority(7, 20);
	heap.dump();

#endif
	 
	return 0;
}

