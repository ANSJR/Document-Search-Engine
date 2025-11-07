
#include <../include/Searcher.h>
#include <iostream>
#include <cmath>
#include <algorithm>


Searcher::Searcher(const std::unordered_map<std::string,
            std::unordered_map<std::string, std::vector<size_t>>>& idx,
            const TernarySearchTree& tst) : index(idx), tst(tst) {}

std::vector<std::pair<std::string, double>> Searcher::search(const std::string& query) {
    std::vector<std::string> queryTokens = tokenizer.tokenize(query);
    if (queryTokens.empty()) return {};

    auto results = chainedPositionalIntersect(index, queryTokens);

    // DEBUG PRINT
    std::cout << "\n\nDEBUGGING PRINT Query (" << query << ") : \n";
    for (const auto& [word, docMap] : results) {
        std::cout << word << ":\n";
        for (const auto& [file, positions] : docMap) {
            std::cout << "  " << file << " -> ";
            for (size_t pos : positions) {
                std::cout << pos << " ";
            }
            std::cout << "\n";
        }
    }
    std::cout << std::endl;

    return {};
    
} 
// Intersect positions of two tokens within the same document
std::vector<size_t> Searcher::positionalIntersect(const std::vector<size_t>& pos1, const std::vector<size_t>& pos2) {
    std::vector<size_t> result;
    size_t i = 0, j = 0;

    while (i < pos1.size() && j < pos2.size()) {
        if (pos1[i] + 1 == pos2[j]) {
            result.push_back(pos2[j]);
            i++;
            j++;
        }
        else if (pos1[i] + 1 < pos2[j]) i++;
        else j++;
    }

    return result;
}
std::unordered_map<std::string,std::unordered_map<std::string, std::vector<size_t>>> Searcher::chainedPositionalIntersect(
const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<size_t>>>& index,
const std::vector<std::string>& queryTokens) {

    using DocPosMap = std::unordered_map<std::string, std::vector<size_t>>;
    std::unordered_map<std::string, DocPosMap> finalResults;

    if (queryTokens.empty()) return finalResults;

    // --- Handle first token (single word or prefix) ---
    std::vector<std::string> firstMatches;
    const std::string& firstToken = queryTokens[0];

    if (index.count(firstToken)) {
        firstMatches.push_back(firstToken);
    } else {
        firstMatches = tst.prefixSearch(firstToken);
        if (firstMatches.empty()) return finalResults;
    }

    // If query has only one token, return all matching words
    if (queryTokens.size() == 1) {
        for (const auto& word : firstMatches) {
            finalResults[word] = index.at(word);
        }
        return finalResults;
    }

    // --- Multi-word query ---
    DocPosMap currentToken;
    bool firstWordSet = false;

    for (const auto& word : firstMatches) {
        auto it = index.find(word);
        if (it != index.end()) {
            currentToken = it->second;
            firstWordSet = true;
            break; // take the first match as starting point
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

        if (nextResults.empty()) return {}; // no matches

        if (targetWords.size() == 1) {
            currentToken = nextResults[targetWords[0]];
        } else {
            // multiple prefix expansions, stop chaining here
            finalResults = nextResults;
            break;
        }
    }

    if (finalResults.empty()) {
        // keep only the last word's results
        std::string lastWord = currentToken.begin()->first;
        return {{ lastWord, currentToken }};
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


// std::map<std::string, double> Searcher::computeScores(std::unordered_map<std::string, std::vector<size_t>>) const {
//     const size_t termCountInDoc     = index.
//     const size_t totalTermsInDoc    = 
//     const size_t docsContainingTerm =
//     const size_t totalDocs          =
//     for(const auto& token : expandedqueryTokens) {

//     }
//     return {};
// }

