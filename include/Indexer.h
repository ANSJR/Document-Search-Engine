/*
 * Indexer.h
 * 
 * Builds and stores an inverted index mapping tokens to document paths.
 * Uses a DocumentReader to load files and a Tokenizer to extract tokens.
 * Core component of the text search engine.
 */

#ifndef INDEXER_H
#define INDEXER_H

#include "DocumentReader.h"
#include "Tokenizer.h"
#include "TernarySearchTree.h"
#include <unordered_map>
#include <vector>
#include <string>

class Indexer {
private:
    DocumentReader& reader;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<size_t>>> index;
public:
    Indexer(DocumentReader& r);
    void buildIndex(const std::vector<std::string>& files);
    void buildIndex(const std::vector<std::string>& files, TernarySearchTree& tst);
    const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<size_t>>>& getIndex() const;
};

#endif
