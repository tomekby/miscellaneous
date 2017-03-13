#include <benchmark/benchmark_api.h>
#include "radix.h"
#include <ctime>
#include <array>
#include <chrono>
#include <random>

#define setIterationTime() auto end = std::chrono::high_resolution_clock::now(); \
auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start); \
state.SetIterationTime(elapsed_seconds.count());

unsigned* getRandomInts(const unsigned count)
{
    // Inicjalizacja generatora liczb losowych
    std::random_device r;
    std::mt19937 gen(r());
    std::uniform_int_distribution<> dis;

    auto res = new unsigned[count];
    for (auto i = 0; i < count; ++i)
        res[i] = dis(gen);
    return res;
}

void HeapPush(benchmark::State& state) {
    auto ints = getRandomInts(state.range(0));

    while (state.KeepRunning()) {
        RadixHeap<unsigned, unsigned> heap(state.range(0));

        auto start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < state.range(0); ++i)
            heap.push(i, ints[i]);
        setIterationTime()
    }
    delete[] ints;

    state.SetLabel("Heap push");
    state.SetComplexityN(state.iterations() * state.range(0));
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(HeapPush)->Range(64, 8 << 16)->UseManualTime()->Complexity();

void HeapPop(benchmark::State& state) {
    auto ints = getRandomInts(state.range(0));

    while(state.KeepRunning()) {
        RadixHeap<unsigned, unsigned> heap(state.range(0));
        for (auto i = 0; i < state.range(0); ++i) 
            heap.push(i, ints[i]);

        auto start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < state.range(0); ++i)
            benchmark::DoNotOptimize(heap.pop());
        setIterationTime()
    }
    delete[] ints;

    state.SetLabel("Heap pop");
    state.SetComplexityN(state.iterations() * state.range(0));
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(HeapPop)->Range(64, 8 << 16)->UseManualTime()->Complexity();

void HeapReducePriority(benchmark::State& state) {
    auto values = getRandomInts(state.range(0));
    auto minimum = std::numeric_limits<unsigned>::max();
    for (auto i = 0; i < state.range(0); ++i)
        minimum = values[i] < minimum ? values[i] : minimum;

    auto processed = 0;
    while (state.KeepRunning()) {
        RadixHeap<unsigned, unsigned> heap(state.range(0));
        for (auto i = 0; i < state.range(0); ++i) 
            heap.push(i, values[i]);

        auto start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < state.range(0); ++i) {
            if (values[i] > minimum) {
                heap.reduce_priority(i, values[i] -= 1);
                ++processed;
            }
        }
        setIterationTime()
    }
    delete[] values;

    state.SetLabel("Heap reduce priority");
    state.SetComplexityN(processed);
    state.SetItemsProcessed(processed);
}
BENCHMARK(HeapReducePriority)->Range(64, 8 << 18)->UseManualTime()->Complexity();

BENCHMARK_MAIN()