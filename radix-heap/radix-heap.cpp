// radix-heap.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "vector.h"
#include "radix.h"
#include <queue>
#include <ctime>

int main()
{
	// Problem #1: warto�ci (elementy, nie priorytety) w kolejce musz� by� unikalne je�li u�ywamy lookup table
	// Problem #2: warto�ci nie s� sprawdzane (np. czy spe�niaj� warunki wstawienia), ale to oszcz�dza par� cykli

	typedef RadixHeap<unsigned, unsigned> rheap;
	rheap::element tmp;

	int count = 500000;
#define _BIG_DEBUG 0
#if _BIG_DEBUG == 0
	rheap heap(count);
	int *numbers = new int[count];
	// klucze to [1,2,3..] a warto�ci to [0,3,6..]
	for (int i = 0; i < count; ++i) numbers[i] = i * 3;

	// Dodawanie
	for (int i = 0; i < count; ++i) {
		heap.push(i, numbers[i]);
	}
	for (int i = 1; i < (count >> 2); ++i) {
		heap.change_priority(i, numbers[i], numbers[i] - 2);
	}
#endif
#if _BIG_DEBUG == 1
					//0  1  2  3 4  5  6  7  8  9 10 11 12 13 14 15 16 17
	int numbers[] = { 7,58,59,13,8,49,51,23,30,16,39,11,10,9, 63,33,48,57};
	count = sizeof(numbers) / sizeof(numbers[0]);

	rheap heap(count);
	for (int i = 0; i < count; ++i) heap.push(i, numbers[i]);
	heap.dump();
	int numbers2[] = { 2, 0, 4 };
	for (int i = 0; i < 3; ++i) heap.change_priority(numbers2[i], numbers[numbers2[i]], numbers[numbers2[i]] - 3);
	heap.dump();
#endif
// Test szybko�ci dla std::queue
#if _BIG_DEBUG == 2
#include <queue>
	std::priority_queue<unsigned> heap;
	for (int i = 0; i < count >> 4; ++i) heap.push(i);
#endif
	// Usuwanie
	while (!heap.empty()) {
		heap.pop();
	}
	printf("\n\nKolejka oprozniona");
	 
	return 0;
}

