#include <benchmark/benchmark.h>

#include "Indexer.h"
#include "Tokenizer.h"

static void BM_Tokenize(benchmark::State& state)
{
    Indexer idx;
    std::string text = idx.readText("benchmarks/WarAndPeace.txt");
    Tokenizer tokenizer;

    for (auto _ : state) {
        auto tokens = tokenizer.tokenize(text);
        benchmark::DoNotOptimize(tokens);
    }
}

BENCHMARK(BM_Tokenize)
    ->Unit(benchmark::kMillisecond);
