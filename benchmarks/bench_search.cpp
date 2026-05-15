#include <benchmark/benchmark.h>
#include "Indexer.h"
#include "Searcher.h"
#include "TernarySearchTree.h"


[[maybe_unused]]
static void printDebugShort(const std::unordered_map<std::string,std::unordered_map<std::filesystem::path,std::vector<WordLocation>>>& results, const Indexer& idx, const std::string& query) {
    std::cout << "DEBUGGING PRINT Query (" << query << ") :\n";
    for (const auto& [word, docMap] : results) {
        std::cout << "\nWORD: " << word << '\n';
        for (const auto& [file, positions] : docMap) {
            
            std::cout
                << "  "
                << file
                << " [gen="
                << idx.getFileGen(file)
                << "] -> ";
            for (const WordLocation& pos : positions) {
                std::cout
                    << "("
                    << pos.tokenPos
                    << ", "
                    << pos.byteOffset
                    << ") ";
            }
            std::cout << '\n';
        }
    }

    std::cout << std::endl;
}
[[maybe_unused]]
static void printDebugLong(const std::unordered_map<std::string,std::unordered_map<std::filesystem::path,std::vector<WordLocation>>>& results, const Indexer& idx, const std::string& query) {
        std::cout
        << "\n======================================================"
        << "\nSEARCH DEBUG REPORT"
        << "\n======================================================\n";

    std::cout << "Query               : " << query << '\n';
    std::cout << "Indexed Terms       : " << idx.getTotalIndexTerms() << '\n';
    
    size_t totalMatchedFiles = 0;
    size_t totalOccurrences = 0;
    size_t totalMatchedTerms = results.size();
    std::unordered_set<std::filesystem::path> uniqueFiles;
    for (const auto& [word, docMap] : results) {
        for (const auto& [file, positions] : docMap) {
            uniqueFiles.insert(file);
            totalOccurrences += positions.size();
        }
    }

    totalMatchedFiles = uniqueFiles.size();
    std::cout << "Matched Terms       : " << totalMatchedTerms << '\n';
    std::cout << "Matched Files       : " << totalMatchedFiles << '\n';
    std::cout << "Total Occurrences   : " << totalOccurrences << '\n';
    std::cout
        << "\n------------------------------------------------------"
        << "\nTERM BREAKDOWN"
        << "\n------------------------------------------------------\n";

    for (const auto& [word, docMap] : results) {
        size_t wordOccurrences = 0;
        for (const auto& [file, positions] : docMap) {
            wordOccurrences += positions.size();
        }
        std::cout
            << "\nWORD: " << word
            << "\nFiles Matched       : " << docMap.size()
            << "\nOccurrences         : " << wordOccurrences
            << "\n";
        for (const auto& [file, positions] : docMap) {
            std::cout
                << "\n  FILE              : " << file
                << "\n  Generation        : " << idx.getFileGen(file)
                << "\n  Match Count       : " << positions.size()
                << "\n  Positions         : ";
            for (const WordLocation& pos : positions) {
                std::cout
                    << "("
                    << pos.tokenPos
                    << ", "
                    << pos.byteOffset
                    << ") ";
            }
            std::cout << "\n";
        }
        std::cout
            << "------------------------------------------------------\n";
    }
    std::cout
        << "\n======================================================"
        << "\nEND DEBUG REPORT"
        << "\n======================================================\n\n";
}
static void BM_Search(benchmark::State& state)
{
    std::filesystem::path path = std::filesystem::weakly_canonical("data");
    std::filesystem::path filePath = std::filesystem::weakly_canonical("data/doc1.txt");
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            auto normalized = std::filesystem::weakly_canonical(entry.path());
            auto ext = normalized.extension();
            if (ext == ".txt" || ext == ".md") {
                files.push_back(normalized);
            }
        }
    }
    static Indexer idx;
    static TernarySearchTree tst;
    static bool initialized = false;

    if (!initialized) {
        idx.buildIndex(files, tst);
        idx.buildIndex(filePath, tst);
        initialized = true;
    }

    static Searcher searcher(idx.getIndex(), tst);

    std::string query = "orange";
    for (auto _ : state) {
        auto results = searcher.search(query);
        benchmark::DoNotOptimize(results);
    }
    
    static bool printed = false;
    if (!printed) {
        auto results = searcher.search(query);
        printDebugShort(results, idx, query);
        printed = true;
    }
}

BENCHMARK(BM_Search)
    ->Unit(benchmark::kMillisecond);