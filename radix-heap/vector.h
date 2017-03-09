#pragma once
#include <cstddef>
#include <ostream>

/**
 * Uproszczona implementacja std::vector
 * Czêœæ podstawowego API zachowana, niepotrzebne elementy niezaimplementowane
 * Dziêki uproszczeniu implementacji jest zauwa¿alnie szybszy od std::vector
 *
 * Autor: Tomasz Stasiak
 */
template<class T>
class vector {
public:
	// Standardowy konstruktor
	vector() : _size(0), _count(0), _buffer(nullptr) {};
	vector(const size_t size) : _size(size != 0 ? size : 1), _count(0) { _buffer = new T[_size]; };
	~vector() {
		delete[] _buffer;
		_buffer = nullptr;
	};
	void assign(const size_t count, const T &value) {
		clear();
		resize(count);
		// Wpisywanie nowych wartoœci
		for (size_t i = 0; i < count; ++i)
			_buffer[i] = value;
	}
	// Gettery
	inline T& operator[](const size_t pos) {
		return *(_buffer + pos);
	};
	// Objêtoœæ
	inline const bool empty() { return _count == 0; }
	inline const size_t size() const { return _count; }
	inline const size_t capacity() { return _size; }
	// Modyfikatory
	inline void clear() {
		delete[] _buffer;
		_buffer = new T[_size = (_count = 0) + _INIT_SIZE];
	}
	void push_back(const T& el) {
		// Leniwa alokacja bufora pozwala zaoszczêdziæ trochê pamiêci
		if (_buffer == nullptr) resize(_INIT_SIZE);
		if (_count == _size) resize(_size * _GROWTH_FACTOR);
		_buffer[_count++] = el;
	}
	// Trochê inny sposób dzia³ania ni¿ w std::vector - zamiast back() i pop_back() pop_back() zwraca zdejmowan¹ wartoœæ
	inline T& pop_back() {
		if (_count != 0) {
			return _buffer[--_count];
		}
		return T();
	}
	// Resize bufora do odpowiedniego rozmiaru
	void resize(size_t new_size) {
		// Resize bufora i przepisanie danych
		T *tmp = new T[new_size];
		if(_buffer != nullptr) memmove(tmp, _buffer, sizeof(_buffer[0]) * _count);

		// Zamiana bufora i czyszczenie po poprzednim
		if (_buffer != nullptr) delete[] _buffer;
		_buffer = tmp;
		_size = new_size;
	}
	void shrink_to_fit() {
		if (_count < _size) reisze(_count);
	}
private:
	// Bufor przechowuj¹cy dane
	T *_buffer;
	// Wielkoœæ bufora
	size_t _size;
	// Rzeczysista iloœæ elementów
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
