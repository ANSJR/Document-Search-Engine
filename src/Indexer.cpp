/*
 * Indexer.cpp
 * 
 * Implements Indexer::buildIndex() to process a list of documents,
 * extract tokens, and populate the inverted index.
 * Prints basic progress information to the console.
 */

#include "../include/Indexer.h"
#include <iostream>

Indexer::Indexer(DocumentReader& r) : reader(r) {}

void Indexer::buildIndex(const std::vector<std::string>& files) {
    long long int totalTokensFiled = 0;
    for (const auto& filepath : files) {
        std::string textString = reader.readText(filepath);
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(textString);
        int i = 0;
        for (const auto& token: tokens) {
            index[token][filepath].push_back(i);
            i++;
        }
        totalTokensFiled += i;
    }
    // std::cout << "Total files " << files.size() << ", tokens filed " << totalTokensFiled << " :" << std::endl;
}
void Indexer::buildIndex(const std::vector<std::string>& files, TernarySearchTree& tst) {
    long long int totalTokensFiled = 0;
    for (const auto& filepath : files) {
        std::string textString = reader.readText(filepath);
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(textString);
        int i = 0;
        for (const auto& token: tokens) {
            index[token][filepath].push_back(i);
            tst.insert(token);
            i++;
        }
        totalTokensFiled += i;
    }
    // std::cout << "Total files " << files.size() << ", tokens filed " << totalTokensFiled << " :" << std::endl;
}

const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<size_t>>>& Indexer::getIndex() const {
    return index;
}