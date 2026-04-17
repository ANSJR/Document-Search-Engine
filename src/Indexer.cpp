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
    for (const auto& filePath : files) {
        // Safety Check
        if(filePresent(filePath)) removeFileFromIndex(filePath, tst);

        std::string textString = readText(filePath);
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(textString);
        for (const auto& [token, loc] : tokens) {
            fileToTerms[filePath].insert(token);
            index[token][filePath].push_back(loc);
            tst.insert(token);
        }
        totalTokensFiled += static_cast<long long>(tokens.size());
    }
    // fileToTerms DEBUG
    // for (const auto& [file, terms] : fileToTerms) {
    //     std::cout << "File: " << file
    //             << " | unique terms: " << terms.size() << "\n";
    //     for (const auto& word : terms) {
    //         std::cout << "  - " << word << "\n";
    //     }
    // }

    // std::cout << "Total files " << files.size() << ", tokens filed "
    //           << totalTokensFiled << " :" << std::endl;
}
void Indexer::buildIndex(const std::string& filePath, TernarySearchTree& tst) {
    if(filePresent(filePath)) {
        removeFileFromIndex(filePath, tst);
    }

    long long int totalTokensFiled = 0;
    std::string textString = readText(filePath);
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize(textString);
    for (const auto& [token, loc] : tokens) {
        fileToTerms[filePath].insert(token);
        index[token][filePath].push_back(loc);
        tst.insert(token);
    }
    totalTokensFiled += static_cast<long long>(tokens.size());
}
void Indexer::removeFileFromIndex(const std::string& filePath, TernarySearchTree& tst) {
    // Safety Check
    if(!filePresent(filePath)) {
        return;
    }
    std::cout << "PERFORMING FILE REMOVAL FROM INDEX\n";
    for (const auto& term : fileToTerms[filePath]) {
        index[term].erase(filePath);
        if (index[term].empty()) {
            index.erase(term);
            // tst.remove(term);
        }
    }
    fileToTerms.erase(filePath);
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
int Indexer::getTotalTerms() const {
    return index.size();
}
bool Indexer::filePresent(const std::string& filePath) const {
    return fileToTerms.find(filePath) != fileToTerms.end();
}