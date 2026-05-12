#include <benchmark/benchmark.h>
#include "Indexer.h"
#include "Searcher.h"
#include "TernarySearchTree.h"

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
static void BM_Search(benchmark::State& state)
{
    // Build index ONCE
    static auto files = getFiles();
    static Indexer idx;
    static TernarySearchTree tst;
    static bool initialized = false;

    if (!initialized) {
        idx.buildIndex(files, tst);
        initialized = true;
    }

    static Searcher searcher(idx.getIndex(), tst);

    std::string query = "Beyond Birthday";

    for (auto _ : state) {
        auto results = searcher.search(query);
        benchmark::DoNotOptimize(results);
    }
}

BENCHMARK(BM_Search)
    ->Unit(benchmark::kMillisecond);