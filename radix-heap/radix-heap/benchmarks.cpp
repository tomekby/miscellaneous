#include <benchmark/benchmark_api.h>
#include "radix.h"
#include <ctime>
#include <array>

void HeapPush(benchmark::State& state) {
    srand(time(nullptr));

    RadixHeap<unsigned, unsigned> heap(state.range(0));
    while (state.KeepRunning()) {
        auto x = rand() % state.range(0);
        heap.push(x, x);
    }

    state.SetLabel("Heap push");
    state.SetComplexityN(state.range(0));
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(HeapPush)->Range(32, 8 << 10)->Complexity();

void HeapPop(benchmark::State& state) {
    // Filling of the queue shouldn't be benchmarked
    const auto count = 1024 * 1024;
    RadixHeap<unsigned, unsigned> heap(count);
    for (auto i = 0; i < count; ++i) {
        auto x = rand() % count;
        heap.push(x, x);
    }

    while(state.KeepRunning()) {
        benchmark::DoNotOptimize(heap.pop());
    }

    state.SetLabel("Heap pop");
    state.SetComplexityN(state.range(0));
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(HeapPop)->Range(32, 8 << 10)->Complexity();

void HeapReducePriority(benchmark::State& state) {
    const auto count = 1024 * 1024;
    // Filling of the queue shouldn't be benchmarked
    RadixHeap<unsigned, unsigned> heap(count);
    auto values = new unsigned[count];
    auto minimum = count;

    for (auto i = 0; i < count; ++i) {
        auto x = rand() % count;
        heap.push(x, x);
        values[i] = x;
        minimum = x < minimum ? x : minimum;
    };

    for (auto i = 0; state.KeepRunning(); i = i % count) {
        if (values[i] > minimum) {
            heap.reduce_priority(values[i], values[i] - 1);
        }
    }

    state.SetLabel("Heap reduce priority");
    state.SetComplexityN(state.range(0));
    state.SetItemsProcessed(state.iterations());

    delete[] values;
}
BENCHMARK(HeapReducePriority)->Range(32, 8 << 10)->Complexity();

BENCHMARK_MAIN()