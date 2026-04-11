
#include "../include/Engine.h"

Engine::Engine() {}
void Engine::indexFiles(const std::vector<std::string>& files) {
    indexer.buildIndex(files, tst);
}
std::vector<SearchResult> Engine::search(const std::string& query) const {
    const auto& index = indexer.getIndex();
    Searcher searcher(index, tst);
    auto results = searcher.search(query);
    for (const auto& [word, docMap] : index) {
        std::cout << "WORD: " << word << "\n";

        for (const auto& [file, positions] : docMap) {
            std::cout << "  FILE: " << file << " -> ";

            for (const auto& pos : positions) {
                std::cout << "("
                          << pos.tokenPos << ", "
                          << pos.byteOffset << ") ";
            }

            std::cout << "\n";
        }

        std::cout << "----------------------\n";
    }

    std::cout << "=======================\n";
    // for (const auto& [doc, score] : results) {
    //     std::cout << "  " << doc << " (score: " << std::fixed << std::setprecision(2) << score << ")\n";
    // }
}
std::vector<std::string> Engine::prefixSearch(const std::string& prefix) const {

}