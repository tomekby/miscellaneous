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
	// Problem #2: co� si� sypie z pami�ci� :v ale dopiero przy pop - czyli pewnie reorganizacja (?)

	typedef RadixHeap<unsigned> rheap;
	rheap heap(20000 << 2);
	rheap::element tmp;
	std::priority_queue<unsigned> queue;

	//int numbers[] = { 7,58,49,59,13,8,13,49,51,7,23,30,16,39,11,10,9,63,33,48,57 };
	const int count = 20000;
	int *numbers = new int[count];
	srand(time(0));
	for (int i = 0; i < count; ++i) numbers[i] = rand() % (count << 2);
	
	// Dodawanie
	for (int i = 0; i < count; ++i) {
		if(!heap.in_heap(numbers[i])) heap.push(numbers[i], i);
	}
	/*for (int i = count / 2; i > count / 2; --i) {
		if (!heap.in_heap(numbers[i])) continue;
		heap.reduce_priority(numbers[i], i, i > 20 ? i - 20 : i + 20);
	}*/

	// Usuwanie
	while (!heap.empty()) {
		tmp = heap.pop();
		//printf("zdjeta wartosc: %d, o priorytecie: %d\n", tmp.value, tmp.key);
	}

	return 0;
}

