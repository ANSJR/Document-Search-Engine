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
#include <filesystem>

#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>



class Indexer {
private:
    unsigned long long int TotalTokensInIndex = 0;
    std::unordered_map<std::filesystem::path, std::pair<size_t,std::unordered_set<std::string>>> fileToTerms;
    std::unordered_map<std::string, std::unordered_map<std::filesystem::path, std::vector<WordLocation>>> index;
public:
    Indexer();
    void buildIndex(const std::vector<std::filesystem::path>& files, TernarySearchTree& tst);
    void buildIndex(const std::filesystem::path& filePath, TernarySearchTree& tst);
    void removeFileFromIndex(const std::filesystem::path& filePath, TernarySearchTree& tst);
    const std::unordered_map<std::string, std::unordered_map<std::filesystem::path, std::vector<WordLocation>>>& getIndex() const;
    std::string readText(const std::filesystem::path& filePath);
    int getTotalIndexTerms() const;
    bool filePresent(const std::filesystem::path& filePath) const;
    double computeScore(const std::filesystem::path& file, const std::string& term) const;
};

#endif
