/*
 * Indexer.cpp
 * 
 * Implements Indexer::buildIndex() to process a list of documents,
 * extract tokens, and populate the inverted index.
 * Prints basic progress information to the console.
 */

#include "../include/Indexer.h"
#include "../include/DocumentReaderFactory.h"
#include <iostream>
#include <memory>

Indexer::Indexer(DocumentReader& r) : reader(r) {}

void Indexer::buildIndex(const std::vector<std::string>& files) {
    long long int totalTokensFiled = 0;

    for (const auto& filepath : files) {
        std::string textString = reader.readText(filepath);
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(textString);
        for (const auto& [token, loc] : tokens) {
            index[token][filepath].push_back(loc);
        }
        totalTokensFiled += static_cast<long long>(tokens.size());
    }
    // std::cout << "Total files " << files.size() << ", tokens filed "
    //           << totalTokensFiled << " :" << std::endl;
}

void Indexer::buildIndex(const std::vector<std::string>& files, TernarySearchTree& tst) {
    long long int totalTokensFiled = 0;
    for (const auto& filepath : files) {
        std::unique_ptr<DocumentReader> reader = DocumentReaderFactory::createReader(filepath);
        std::string textString = reader->readText(filepath);

        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(textString);
        for (const auto& [token, loc] : tokens) {
            index[token][filepath].push_back(loc);
            tst.insert(token);
        }
        totalTokensFiled += static_cast<long long>(tokens.size());
    }

    // std::cout << "Total files " << files.size() << ", tokens filed "
    //           << totalTokensFiled << " :" << std::endl;
}

const std::unordered_map<std::string,std::unordered_map<std::string, std::vector<WordLocation>>>& Indexer::getIndex() const {
    return index;
}