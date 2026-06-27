#include <benchmark/benchmark.h>
#include "Indexer.h"
#include "IndexSerializer.h"

static void BM_ReadText(benchmark::State& state)
{
    Indexer idx;
    std::filesystem::path file = "benchmarks/WarAndPeace.txt";

    for (auto _ : state) {
        auto text = idx.readText(file);
        benchmark::DoNotOptimize(text);
    }
}

BENCHMARK(BM_ReadText)
    ->Unit(benchmark::kMillisecond);
// make bench FILTER=BM_SingleFileIndexing
static void BM_SingleFileIndexingNoSerialization(benchmark::State& state)
{
    std::filesystem::path file = "benchmarks/WarAndPeace.txt";

    for (auto _ : state) {
        Indexer idx;
        TernarySearchTree tst;
        idx.buildIndex(file, tst);

        benchmark::DoNotOptimize(idx);
        benchmark::DoNotOptimize(tst);
    }
}

BENCHMARK(BM_SingleFileIndexingNoSerialization)
    ->MinTime(5.0)
    ->Unit(benchmark::kMillisecond);

static void BM_SingleFileIndexingWithSerialization(benchmark::State& state)
{
    std::filesystem::path file = "benchmarks/WarAndPeace.txt";

    std::filesystem::remove_all("SingleFileIndexingBin");
    std::filesystem::create_directories("SingleFileIndexingBin");

    for (auto _ : state) {
        Indexer idx;
        TernarySearchTree tst;

        idx.buildIndex(file, tst);

        std::filesystem::path output = std::filesystem::path("SingleFileIndexingBin") / (file.filename().string() + ".bin");

        if (!IndexSerializer::save(idx, file, output)) {
            state.SkipWithError("Failed to serialize index");
            return;
        }

        benchmark::DoNotOptimize(idx);
        benchmark::DoNotOptimize(tst);
    }
}

BENCHMARK(BM_SingleFileIndexingWithSerialization)
    ->MinTime(5.0)
    ->Unit(benchmark::kMillisecond);
//
// Bulk Index Benchmark
//

static std::vector<std::filesystem::path> getFiles() {
    std::filesystem::path path = "benchmarks/BenchDocs";
    // std::filesystem::path path = "GutenbergText";
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

static void BM_BulkIndexingNoSerialization(benchmark::State& state) {
    static auto allFiles = getFiles();
    int fileCount = std::min(static_cast<int>(state.range(0)), static_cast<int>(allFiles.size()));
    std::vector<std::filesystem::path> selectedFiles(allFiles.begin(), allFiles.begin() + fileCount);

    for (auto _ : state) {
        Indexer idx;
        TernarySearchTree tst;

        idx.buildIndex(selectedFiles, tst);

        benchmark::DoNotOptimize(idx);
        benchmark::DoNotOptimize(tst);
    }
}

BENCHMARK(BM_BulkIndexingNoSerialization)
    // ->Arg(1)
    // ->Arg(10)
    ->Arg(100)
    // ->Iterations(10)
    // ->Arg(getFiles().size())
    ->MinTime(5.0)
    ->Unit(benchmark::kMillisecond);

static void BM_BulkIndexingWithSerialization(benchmark::State& state) {
    static auto allFiles = getFiles();
    int fileCount = std::min(static_cast<int>(state.range(0)), static_cast<int>(allFiles.size()));
    std::vector<std::filesystem::path> selectedFiles(allFiles.begin(), allFiles.begin() + fileCount);

    std::filesystem::remove_all("BulkIndexingBin");
    std::filesystem::create_directories("BulkIndexingBin");

    for (auto _ : state) {
        Indexer idx;
        TernarySearchTree tst;

        idx.buildIndex(selectedFiles, tst);

        for (const auto& file : selectedFiles) {
            std::filesystem::path output = std::filesystem::path("BulkIndexingBin") / (file.filename().string() + ".bin");

            if (!IndexSerializer::save(idx, file, output)) {
                state.SkipWithError("Failed to serialize index");
                return;
            }
        }

        benchmark::DoNotOptimize(idx);
        benchmark::DoNotOptimize(tst);
    }
}

BENCHMARK(BM_BulkIndexingWithSerialization)
    // ->Arg(1)
    ->Arg(100)
    ->MinTime(5.0)
    ->Unit(benchmark::kMillisecond);
