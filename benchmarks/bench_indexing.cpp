#include <benchmark/benchmark.h>
#include "Indexer.h"


static void BM_SingleFileIndexing(benchmark::State& state)
{
    std::filesystem::path file = "benchmarks/BenchDocs/BBcase1.txt";

    for (auto _ : state) {
        Indexer idx;
        TernarySearchTree tst;
        idx.buildIndex(file, tst);

        benchmark::DoNotOptimize(idx);
        benchmark::DoNotOptimize(tst);
    }
}

BENCHMARK(BM_SingleFileIndexing)
    ->MinTime(10.0)
    ->Unit(benchmark::kMillisecond);
//
// Bulk Index Benchmark
//

static std::vector<std::filesystem::path> getFiles() {
    std::filesystem::path path = "benchmarks/BenchDocs";
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension();
            if (ext == ".txt" || ext == ".md") {
                files.push_back(entry.path());
            }
        }
    }
    return files;
}

static void BM_BulkIndexing(benchmark::State& state) {
    static auto allFiles = getFiles();
    int fileCount = state.range(0);
    fileCount = std::min(fileCount, static_cast<int>(allFiles.size()));
    std::vector<std::filesystem::path> selectedFiles(allFiles.begin(), allFiles.begin() + fileCount);

    for (auto _ : state) {
        Indexer idx;
        TernarySearchTree tst;
        idx.buildIndex(selectedFiles, tst);
        benchmark::DoNotOptimize(idx);
        benchmark::DoNotOptimize(tst);
    }
}
BENCHMARK(BM_BulkIndexing)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->MinTime(10.0)
    ->Unit(benchmark::kMillisecond);

static void BM_ReadText(benchmark::State& state)
{
    Indexer idx;
    std::filesystem::path file = "benchmarks/BenchDocs/BBcase1.txt";

    for (auto _ : state) {
        auto text = idx.readText(file);
        benchmark::DoNotOptimize(text);
    }
}

BENCHMARK(BM_ReadText)
    ->Unit(benchmark::kMillisecond);