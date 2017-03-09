#pragma once
/**
* Swap 2 zmiennych
* Wymaga istnienia operatora przypisania dla typu 'type'
*
* @template type typ zamienianych zmiennych
* @param    a pierwsza zmienna
* @param    b druga zmienna
* @return   void
*/
template <class type>
void swap(type &a, type &b) {
	type tmp = a;
	a = b;
	b = tmp;
}

/**
* Swap dla intów bez zmiennych tymczasowych
*/
template <>
void swap(int &a, int &b) {
	a ^= b; b ^= a; a ^= b;
}

/**
* Zamiana wartoœci zmiennych wskazywanych przez pointery
* Wymaga istnienia operatora przypisania dla typu 'type'
*
* @template type typ zamienianych zmiennych
* @param    a pointer do pierwszej zmiennej
* @param    b pointer do drugiej zmiennej
* @return   void
*/
template <class type>
void iter_swap(type *&a, type *&b) {
	swap(*a, *b);
}

/**
* Quicksort z w³asnym komparatorem
*
* @template type typ zmiennych przechowywanych w tablicy/kontenerze
* @template compare typ komparatora
* @param    first wkaŸnik do pierwszego elementu sortowanego przedzia³u
* @param    last pointer do ostatniego elementu sortowanego przedzia³u
* @param    cmp komparator
* @return   void
*/
template<class type, class compare>
void quick_sort(type *first, type *last, compare cmp) {
	if (first != last) {
		type *left = first, *right = last, *pivot = left++;
		while (left != right) {
			if (cmp(*left, *pivot)) {
				++left;
			}
			else {
				while ((left != right) && cmp(*pivot, *right))
					--right;
				iter_swap(left, right);
			}
		}
		--left;
		iter_swap(pivot, left);

		quick_sort(first, left, cmp);
		quick_sort(right, last, cmp);
	}
}

/**
* Quicksort wykorzystuj¹cy prze³adowany operator porównania
*
* @template type typ danych przechowywany w tablicy
* @param    first wkaŸnik do pierwszego elementu sortowanego przedzia³u
* @param    last pointer do ostatniego elementu sortowanego przedzia³u
* @return   void
*/
template<class type>
void quick_sort(type *first, type* last) {
	quick_sort(first, last, operator<);
}

/**
* Najbardziej elementarne algorytmy
* Autor: Tomasz Stasiak
*/