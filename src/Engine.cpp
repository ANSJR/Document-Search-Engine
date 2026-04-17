
#include "../include/Engine.h"

Engine::Engine() {}
void Engine::indexFiles(const std::vector<std::string>& files) {
    indexer.buildIndex(files, tst);
    // const auto& index = indexer.getIndex();
    // for (const auto& [word, docMap] : index) {
    //     std::cout << "WORD: " << word << "\n";

    //     for (const auto& [file, positions] : docMap) {
    //         std::cout << "  FILE: " << file << " -> ";
    //         for (const auto& pos : positions) {
    //             std::cout << "(" << pos.tokenPos << ", " << pos.byteOffset << ") ";
    //         }
    //         std::cout << "\n";
    //     }
    // }
    // std::cout << "=======================";
}
void Engine::indexFile(const std::string& filePath) {
    indexer.buildIndex(filePath, tst);
}
std::vector<SearchResult> Engine::search(const std::string& query) const {
    const auto& index = indexer.getIndex();
    Searcher searcher(index, tst);
    auto results = searcher.search(query);
    std::vector<SearchResult> finalResults;
    if (results.empty()) std::cout << "EMPTY!!!!!!!!!";

    //  Check if a prefix search
    if (results.begin()->first != tokenizer.isolateLastToken(query)) {
        std::cout << "\nPERFORMING PREFIX SEARCH\n\n";
    }
    std::unordered_map<std::string, SearchResult> fileMap;
    for (const auto& [word, fileMapInner] : results) {
        for (const auto& [file, locations] : fileMapInner) {
            // create SearchResult if none exist
            if (fileMap.find(file) == fileMap.end()) {
                fileMap[file] = SearchResult{file, 0.0,{}};
            }
            TermMatch termMatch;
            termMatch.term = word;
            for (const auto& loc : locations) {
                MatchOccurrence occ;
                occ.tokenPos = loc.tokenPos;
                occ.byteOffset = loc.byteOffset;
                occ.length = word.size();
                termMatch.occurrences.push_back(occ);
            }
            // add term to file
            fileMap[file].termMatches.push_back(termMatch);
        }
    }
    for (auto& [file, result] : fileMap) {
        finalResults.push_back(result);
    }
    printSearchResults(finalResults);
    auto scoredResults = searcher.computeScores(results);
    // for (const auto& [doc, score] : scoredResults) {
    //     std::cout << "  " << doc << " (score: " << std::fixed << std::setprecision(2) << score << ")\n";
    // }
    return finalResults;
}
void Engine::printSearchResults(const std::vector<SearchResult>& results) const{
    if (results.empty()) {
        std::cout << "No results found.\n";
        return;
    }

    for (const auto& result : results) {
        std::cout << "FILE: " << result.file << "\n";
        std::cout << "SCORE: " << result.score << "\n";

        for (const auto& termMatch : result.termMatches) {
            std::cout << "  TERM: " << termMatch.term << "\n";
            for (const auto& occ : termMatch.occurrences) {
                std::cout << "    (tokenPos: " << occ.tokenPos
                          << ", byteOffset: " << occ.byteOffset
                          << ", length: " << occ.length << ")\n";
            }
        }
        std::cout << "-------------------------\n";
    }
}
int Engine::getTotalTerms() const{
    return indexer.getTotalTerms();
}