
#include <../include/Searcher.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>


Searcher::Searcher(const std::unordered_map<std::string,
            std::unordered_map<std::string, std::vector<WordLocation>>>& idx,
            const TernarySearchTree& tst) : index(idx), tst(tst) {}

std::vector<std::pair<std::string, double>> Searcher::search(const std::string& query) {
    std::vector<std::string> queryTokens = tokenizer.simpleTokenize(query);
    if (queryTokens.empty()) return {};

    auto results = chainedPositionalIntersect(index, queryTokens);

    std::cout << "\n\nDEBUGGING PRINT Query (" << query << ") : \n";
    for (const auto& [word, docMap] : results) {
        for (const auto& [file, positions] : docMap) {
            std::cout << "  " << file << " -> ";
            for (const WordLocation& pos : positions) {
                std::cout << pos.tokenPos << " ";
            }
            std::cout << "\n";
        }
    }
    std::cout << std::endl;

    auto rankedResults = computeScores(results);
    return rankedResults;
}
// Intersect positions of two tokens within the same document
std::vector<WordLocation> Searcher::positionalIntersect(const std::vector<WordLocation>& pos1, const std::vector<WordLocation>& pos2) {
    std::vector<WordLocation> result;
    size_t i = 0, j = 0;

    while (i < pos1.size() && j < pos2.size()) {
        if (pos1[i].tokenPos + 1 == pos2[j].tokenPos) {
            result.push_back(pos2[j]);
            i++;
            j++;
        }
        else if (pos1[i].tokenPos + 1 < pos2[j].tokenPos) i++;
        else j++;
    }
    return result;
}
std::unordered_map<std::string, std::unordered_map<std::string, std::vector<WordLocation>>>
Searcher::chainedPositionalIntersect(const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<WordLocation>>>& index, const std::vector<std::string>& queryTokens
) {
    using DocPosMap = std::unordered_map<std::string, std::vector<WordLocation>>;
    std::unordered_map<std::string, DocPosMap> finalResults;

    if (queryTokens.empty()) return finalResults;

    std::vector<std::string> firstMatches;
    const std::string& firstToken = queryTokens[0];

    if (index.count(firstToken)) {
        firstMatches.push_back(firstToken);
    } else {
        firstMatches = tst.prefixSearch(firstToken);
        if (firstMatches.empty()) return finalResults;
    }

    if (queryTokens.size() == 1) {
        for (const auto& word : firstMatches) {
            finalResults[word] = index.at(word);
        }
        return finalResults;
    }

    DocPosMap currentToken;
    std::string currentWord;
    bool firstWordSet = false;

    for (const auto& word : firstMatches) {
        auto it = index.find(word);
        if (it != index.end()) {
            currentToken = it->second;
            currentWord = word;
            firstWordSet = true;
            break;
        }
    }
    if (!firstWordSet) return finalResults;

    for (size_t i = 1; i < queryTokens.size(); ++i) {
        const std::string& token = queryTokens[i];
        std::vector<std::string> targetWords;

        if (index.count(token)) {
            targetWords.push_back(token);
        } else {
            targetWords = tst.prefixSearch(token);
            if (targetWords.empty()) return finalResults;
        }

        std::unordered_map<std::string, DocPosMap> nextResults;

        for (const auto& nextWord : targetWords) {
            auto nextIt = index.find(nextWord);
            if (nextIt == index.end()) continue;

            const DocPosMap& nextMap = nextIt->second;
            DocPosMap resultingFileAndPositions;

            for (const auto& [file, pos1] : currentToken) {
                auto found = nextMap.find(file);
                if (found != nextMap.end()) {
                    auto newPositions = positionalIntersect(pos1, found->second);
                    if (!newPositions.empty()) {
                        resultingFileAndPositions[file] = std::move(newPositions);
                    }
                }
            }

            if (!resultingFileAndPositions.empty()) {
                nextResults[nextWord] = resultingFileAndPositions;
            }
        }
        if (nextResults.empty()) return {};
        if (targetWords.size() == 1) {
            currentWord = targetWords[0];
            currentToken = nextResults[targetWords[0]];
        } else {
            finalResults = nextResults;
            break;
        }
    }

    if (finalResults.empty()) {
        return {{currentWord, currentToken}};
    }
    return finalResults;
}



// will return a unsorted vector of this prefix partial token.
std::vector<std::string> Searcher::expandToken(const std::string& token) const {
    return {};
}

double Searcher::computeTF(size_t termCountInDoc, size_t totalTermsInDoc) const {
    return (static_cast<double>(termCountInDoc) / totalTermsInDoc);
}

double Searcher::computeIDF(size_t docsContainingTerm, size_t totalDocs) const {
    return (log((static_cast<double>(totalDocs)) / (1.0 + docsContainingTerm)));
}


std::vector<std::pair<std::string, double>> Searcher::computeScores(const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<WordLocation>>>& results) const {
    std::vector<std::pair<std::string, double>> rankedDocs; // (doc, score)
    size_t totalDocs = index.size();

    for (const auto& [word, docMap] : results) {
        size_t docsContaining = docMap.size();
        double idf = std::log((double)totalDocs / (1.0 + docsContaining));

        for (const auto& [doc, positions] : docMap) {
            double tf = static_cast<double>(positions.size());
            double score = tf * idf; // basic TF-IDF
            rankedDocs.emplace_back(doc, score);
        }
    }

    std::sort(rankedDocs.begin(), rankedDocs.end(),[](const auto& a, const auto& b) {return a.second > b.second;});
    return rankedDocs;
}
// NEEDS OPTIMIZING 
void Searcher::printWordContext(const std::string& filepath, size_t tokenIndex, size_t window) {
    // std::ifstream file(filepath);
    // if (!file) return;

    // std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Tokenizer tokenizer;
    // auto tokens = tokenizer.tokenize(text);

    // if (tokenIndex >= tokens.size()) return;

    // size_t start = (tokenIndex >= window) ? tokenIndex - window : 0;
    // size_t end = std::min(tokenIndex + window, tokens.size() - 1);

    // std::cout << "... ";
    // for (size_t i = start; i <= end; ++i) {
    //     if (i == tokenIndex)
    //         std::cout << "[" << tokens[i] << "] ";
    //     else
    //         std::cout << tokens[i] << " ";
    // }
    // std::cout << "...\n";
}

