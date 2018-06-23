#define CACHE_MIN 0
#include <benchmark/benchmark_api.h>
#include "radix.h"
#include "dheap.h"
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

void RHeapPush(benchmark::State& state) {
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
BENCHMARK(RHeapPush)->Range(64, 8 << 16)->UseManualTime()->Complexity();

void RHeapPop(benchmark::State& state) {
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
BENCHMARK(RHeapPop)->Range(64, 8 << 16)->UseManualTime()->Complexity();

void RHeapReducePriority(benchmark::State& state) {
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
BENCHMARK(RHeapReducePriority)->Range(64, 8 << 18)->UseManualTime()->Complexity();


/**
 * D-ary heap for comparision
 */
template<class Q>
void DHeapReducePriority(benchmark::State& state) {
    auto values = getRandomInts(state.range(0));
    auto minimum = std::numeric_limits<unsigned>::max();
    for (auto i = 0; i < state.range(0); ++i)
        minimum = values[i] < minimum ? values[i] : minimum;

    auto processed = 0;
    while (state.KeepRunning()) {
        Q heap(state.range(0));
        for (auto i = 0; i < state.range(0); ++i)
            heap.push(i, values[i]);
        heap.build_heap();

        auto start = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < state.range(0); ++i) {
            if (values[i] > minimum) {
                heap.change_priority(i, values[i] -= 1);
                ++processed;
            }
        }
        setIterationTime()
    }
    delete[] values;

    state.SetLabel("D-ary reduce priority");
    state.SetComplexityN(processed);
    state.SetItemsProcessed(processed);
}
BENCHMARK_TEMPLATE(DHeapReducePriority, dheap<>)->Range(64, 8 << 16)->UseManualTime()->Complexity();
BENCHMARK_TEMPLATE(DHeapReducePriority, dheap<3>)->Range(64, 8 << 16)->UseManualTime()->Complexity();
BENCHMARK_TEMPLATE(DHeapReducePriority, dheap<4>)->Range(64, 8 << 16)->UseManualTime()->Complexity();
BENCHMARK_TEMPLATE(DHeapReducePriority, dheap<5>)->Range(64, 8 << 16)->UseManualTime()->Complexity();


BENCHMARK_MAIN()