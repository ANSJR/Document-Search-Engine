/*
 * Indexer.cpp
 *
 * Implements Indexer::buildIndex() to process a list of documents,
 * extract tokens, and populate the inverted index.
 * Prints basic progress information to the console.
 */

#include "../include/Indexer.h"
#include <cmath>

Indexer::Indexer() {}

void Indexer::buildIndex(const std::vector<std::string>& files) {
    // long long int totalTokensFiled = 0;

    for (const auto& filepath : files) {
        std::string textString = readText(filepath);
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(textString);
        for (const auto& [token, loc] : tokens) {
            index[token][filepath].push_back(loc);
        }
        // totalTokensFiled += static_cast<long long>(tokens.size());
    }
    // std::cout << "Total files " << files.size() << ", tokens filed "
    //           << totalTokensFiled << " :" << std::endl;
}

void Indexer::buildIndex(const std::vector<std::string>& files, TernarySearchTree& tst) {
    // long long int totalTokensFiled = 0;
    for (const auto& filePath : files) {
        // Safety Check
        if(filePresent(filePath)) removeFileFromIndex(filePath, tst);

        std::string textString = readText(filePath);
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(textString);
        fileToTerms[filePath].first = tokens.size();
        for (const auto& [token, loc] : tokens) {
            if (index.find(token) == index.end()) {
                tst.insert(token);
            }
            fileToTerms[filePath].second.insert(token);
            index[token][filePath].push_back(loc);
        }
        // totalTokensFiled += static_cast<long long>(tokens.size());
    }
    // fileToTerms DEBUG
    // for (const auto& [file, terms] : fileToTerms.second) {
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

    // long long int totalTokensFiled = 0;
    std::string textString = readText(filePath);
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize(textString);
    fileToTerms[filePath].first = tokens.size();
    for (const auto& [token, loc] : tokens) {
        if (index.find(token) == index.end()) {
            tst.insert(token);
        }
        fileToTerms[filePath].second.insert(token);
        index[token][filePath].push_back(loc);
    }
    // totalTokensFiled += static_cast<long long>(tokens.size());
}
void Indexer::removeFileFromIndex(const std::string& filePath, TernarySearchTree& tst) {
    // Safety Check
    if(!filePresent(filePath)) {
        return;
    }

    std::cout << "PERFORMING FILE REMOVAL FROM INDEX\n";
    for (const auto& term : fileToTerms[filePath].second) {
        index[term].erase(filePath);
        if (index[term].empty()) {
            index.erase(term);
            tst.deleteTerm(term);
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
int Indexer::getTotalIndexTerms() const {
    return index.size();
}
bool Indexer::filePresent(const std::string& filePath) const {
    return fileToTerms.find(filePath) != fileToTerms.end();
}
double Indexer::computeScore(const std::string& file, const std::string& term) const {
    double N = fileToTerms.size(); // Total docs
    double df = index.at(term).size(); // Docs containing term
    double tf = index.at(term).at(file).size(); // Times term appears in this doc
    double docLen = fileToTerms.at(file).first; // Current doc token count
    double curAvgDocLen = getTotalIndexTerms() / static_cast<double>(fileToTerms.size()); // Average token count across docs
    double k1 = 1.5; // TF tuning constant : Controls how much extra benefit repeated term matches give. So higher k1 = repeated occurrences matter more
    double b = 0.75; // Doc-length normalization constant : Controls how much document length penalizes/normalizes the score. b = 0.0 (ignore doc length) and b = 1.0 (full length normalization)

    double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);
    double numerator = tf * (k1 + 1.0);
    double denominator = tf + k1 * (1.0 - b + b * (docLen / curAvgDocLen));

    double score = idf * (numerator / denominator);
    // std::cout << "\nfile: " << file
    //       << "\nterm: " << term
    //       << "\nN: " << N
    //       << "\ndf: " << df
    //       << "\ntf: " << tf
    //       << "\ndocLen: " << docLen
    //       << "\navgDocLen: " << curAvgDocLen
    //       << "\nidf: " << idf
    //       << "\nscore: " << score << "\n";

    return score;
}