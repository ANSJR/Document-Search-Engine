#include <benchmark/benchmark.h>
#include "Indexer.h"
#include "TernarySearchTree.h"
#include "IndexSerializer.h"

[[maybe_unused]]
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
[[maybe_unused]]
static void prepareSerializedIndex()
{
    std::filesystem::remove_all("LoadSerializedIndexBin");
    std::filesystem::create_directories("LoadSerializedIndexBin");
    auto files = getFiles();
    Indexer idx;
    TernarySearchTree tst;
    idx.buildIndex(files, tst);
    for (const auto& file : files) {
        std::filesystem::path output = std::filesystem::path("LoadSerializedIndexBin") / (file.filename().string() + ".bin");

        if (!IndexSerializer::save(idx, file, output)) {
            throw std::runtime_error("Failed to serialize " + file.string());
        }
    }
}

static void BM_LoadSerializedIndex(benchmark::State& state)
{
    static bool prepared = false;
    if (!prepared) {
        prepareSerializedIndex();
        prepared = true;
    }

    for (auto _ : state) {
        Indexer idx;
        TernarySearchTree tst;
        IndexSerializer::load(idx, tst, "LoadSerializedIndexBin");
        benchmark::DoNotOptimize(idx);
        benchmark::DoNotOptimize(tst);
    }
}

BENCHMARK(BM_LoadSerializedIndex)
    ->MinTime(5.0)
    ->Unit(benchmark::kMillisecond);