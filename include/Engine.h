#ifndef ENGINE_H
#define ENGINE_H
#include "../include/SearchTypes.h"
#include "../include/Indexer.h"
#include "../include/Searcher.h"
#include "../include/TernarySearchTree.h"
#include "../include/Tokenizer.h"


class Engine {
private:
    TernarySearchTree tst;
    Indexer indexer;
    // Tokenizer tokenizer;

public:
    Engine();
    void indexFiles(const std::vector<std::string>& files);
    void indexFile(const std::string& filePath);
    void deleteTermFromFile(const std::string& filePath);
    std::vector<SearchResult> search(const std::string& query) const;
    int getTotalIndexTerms() const;
    int getTotalTreeTerms() const;
};
#endif