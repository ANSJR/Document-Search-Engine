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
    std::unordered_map<std::string, std::unordered_set<std::string>> fileToTerms;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<WordLocation>>> index;
public:
    Indexer();
    void buildIndex(const std::vector<std::string>& files);
    void buildIndex(const std::vector<std::string>& files, TernarySearchTree& tst);
    void buildIndex(const std::string& filePath, TernarySearchTree& tst);
    void removeFileFromIndex(const std::string& filePath, TernarySearchTree& tst);
    const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<WordLocation>>>& getIndex() const;
    std::string readText(const std::string& filePath);
    int getTotalTerms() const;
    bool filePresent(const std::string& filePath) const;
};

#endif
