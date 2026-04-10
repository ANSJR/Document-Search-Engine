/*
 * Indexer.cpp
 * 
 * Implements Indexer::buildIndex() to process a list of documents,
 * extract tokens, and populate the inverted index.
 * Prints basic progress information to the console.
 */

#include "../include/Indexer.h"

Indexer::Indexer() {}

void Indexer::buildIndex(const std::vector<std::string>& files) {
    long long int totalTokensFiled = 0;

    for (const auto& filepath : files) {
        std::string textString = readText(filepath);
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
        std::string textString = readText(filepath);

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

std::string Indexer::readText(const std::string& filePath) {
    std::ifstream file(filePath);
    std::ostringstream buffer; // in-memory stream, dynamically sized std::string that is stored internally
    buffer << file.rdbuf(); // reads data from source buffer and writes to internally stored string
    return buffer.str(); // buffer.str returns finalized internally stored string
}