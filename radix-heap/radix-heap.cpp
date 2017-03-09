// radix-heap.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "vector.h"
#include "radix.h"
#include <intrin.h>

int main()
{
	typedef RadixHeap<unsigned> rheap;
	rheap heap;
	rheap::element tmp;

	int numbers[] = { 7,58,49,59,13,8,13,49,51,7,23,30,16,39,11,10,9,63,33,48,57 };
	// Dodawanie
	for (int i : numbers) heap.push(i, i);
	heap.dump();
	// Usuwanie
	while (!heap.empty()) {
		tmp = heap.pop();
		printf("zdjeta wartosc: %d, o priorytecie: %d\n", tmp.value, tmp.key);
	}

	return 0;
}

