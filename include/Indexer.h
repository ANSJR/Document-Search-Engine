/*
 * Indexer.h
 * 
 * Builds and stores an inverted index mapping tokens to document paths.
 * Loads files and a Tokenizer to extract tokens.
 * Core component of the text search engine.
 */

#ifndef INDEXER_H
#define INDEXER_H
#include "Tokenizer.h"
#include "TernarySearchTree.h"
#include "WordLocation.h"
#include "IndexSerializer.h"
#include <filesystem>

#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
// std::pair<size_t,std::unordered_set<std::string>>
struct FileMetadata {
    size_t tokenCount;
    std::unordered_set<std::string> uniqueTerms;
    uint64_t generation = 0;
};
using LocalIndex = std::unordered_map<std::string,std::vector<WordLocation>>;
using LocalFileToTerms = FileMetadata;
struct PartialResult {
    std::filesystem::path filePath;
    LocalIndex localIndex;
    LocalFileToTerms localFileToTerms;
};
class Indexer {
friend class IndexSerializer;
private:
    unsigned long long int totalTokensInIndex = 0;
    std::unordered_map<std::filesystem::path, FileMetadata> fileToTerms;
    std::unordered_map<std::string, std::unordered_map<std::filesystem::path, std::vector<WordLocation>>> index;
    PartialResult partialIndexThreadWorkers(const std::filesystem::path& files);
    void mergePartialIndexThreadWorkers(PartialResult&& partial, TernarySearchTree& tst);
public:
    Indexer();
    void buildIndex(const std::vector<std::filesystem::path>& files, TernarySearchTree& tst);
    void buildIndex(const std::filesystem::path& filePath, TernarySearchTree& tst);
    void removeFileFromIndex(const std::filesystem::path& filePath, TernarySearchTree& tst);
    const std::unordered_map<std::string, std::unordered_map<std::filesystem::path, std::vector<WordLocation>>>& getIndex() const;
    std::string readText(const std::filesystem::path& filePath);
    int getTotalIndexTerms() const;
    uint64_t getFileGen(const std::filesystem::path& filePath) const;
    bool filePresent(const std::filesystem::path& filePath) const;
    double computeScore(const std::filesystem::path& file, const std::string& term) const;
};

#endif
