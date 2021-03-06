#pragma once
#include <cstddef>
#include <ostream>
#include <cassert>

/**
 * Czy kontener ma automatycznie zmniejsza� swoj� obj�to�� wraz z ubytkiem element�w?
 * Znacznie spowalnia, ale oszcz�dza pami��, kt�ra jest zwalniana z niszczeniem kontenera
 */
#ifndef AUTO_RESIZE_DOWN
	#define AUTO_RESIZE_DOWN 0
#endif

/**
 * Uproszczona implementacja std::vector
 * Cz�� podstawowego API zachowana, niepotrzebne elementy niezaimplementowane
 * Dzi�ki uproszczeniu implementacji jest zauwa�alnie szybszy od std::vector
 *
 * Autor: Tomasz Stasiak
 */
template<class T>
class vector {
public:
	// Standardowy konstruktor
	vector() : _size(_INIT_SIZE) { _count = 0; _buffer = new T[_size]; };
	vector(const size_t size) : _size(size != 0 ? size : 1) { _buffer = new T[_size]; _count = 0; };
	~vector() {
		delete[] _buffer;
		_buffer = nullptr;
	};
	void assign(const size_t count, const T &value) {
		clear();
		resize(count);
		// Wpisywanie nowych warto�ci
		for (size_t i = 0; i < count; ++i)
			_buffer[i] = value;
	}
	// Gettery
    T& operator[](const size_t pos) {
		return _buffer[pos];
	}
	// Obj�to��
    bool empty() const { return _count == 0; }
    size_t size() const { return _count; }
    size_t capacity() const { return _size; }
	// Modyfikatory
	void clear() {
		delete[] _buffer;
		_count = 0;
		_buffer = new T[_size = _INIT_SIZE];
	}
	void push_back(const T& el) {
		if (_count == _size) resize(_size * _GROWTH_FACTOR);
		_buffer[_count++] = el;
	}
	// Troch� inny spos�b dzia�ania ni� w std::vector - zamiast back() i pop_back() pop_back() zwraca zdejmowan� warto��
	T& pop_back() {
#if AUTO_RESIZE_DOWN
		// Zmniejszenie obj�to�ci wektora je�li nie jest potrzebna
		if (_size / _GROWTH_FACTOR > _count * 2) resize(_size / _GROWTH_FACTOR);
#endif
		assert(_count > 0);
		return _buffer[--_count];
	}
	T& back() {
		return _buffer[_count - 1];
	}
	// Resize bufora do odpowiedniego rozmiaru
	void resize(size_t new_size) {
		if (new_size <= _INIT_SIZE) new_size = _INIT_SIZE;
		// Resize bufora i przepisanie danych
		T *tmp = new T[new_size];
		for (size_t i = 0; i < _count; ++i) tmp[i] = _buffer[i];

		// Zamiana bufora i czyszczenie po poprzednim
		delete[] _buffer;
		_buffer = tmp;
		_size = new_size;
	}
protected:
	// Bufor przechowuj�cy dane
	T *_buffer;
	// Wielko�� bufora
	size_t _size;
	// Rzeczysista ilo�� element�w
	size_t _count;
	const static size_t _GROWTH_FACTOR = 2;
	const static size_t _INIT_SIZE = 10;
};


namespace std {
	// Specjalizacja swap dla wektora
	template<class T>
	void swap(::vector<T> &a, ::vector<T> &b) {}

	// Specjalizacja wypisania dla wektora 
	// JSON format
	template<class T>
	ostream& operator<<(std::ostream &s, ::vector<T> &v) {
		s << "[";
		for (size_t i = 0; i < v.size(); ++i)
			s << v[i] << (i != v.size() - 1 ? "," : "");
		return s << "]";
	}
}
