
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
    std::cout << "\n\nDEBUGGING PRINT w/ file and pos of searched : \n";
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

    using DocPosMap = std::unordered_map<std::string, std::vector<size_t>>; // Shorting the code
    std::unordered_map<std::string, DocPosMap> finalResults; // word -> {doc -> pos}

    if (queryTokens.empty()) return finalResults;

    auto firstIt = index.find(queryTokens[0]);
    if (firstIt == index.end()) return finalResults;

    DocPosMap currentToken = firstIt->second;

    // Iterate remaining tokens
    for (size_t i = 1; i < queryTokens.size(); ++i) {
        const std::string& token = queryTokens[i];
        std::vector<std::string> targetWords;

        // Case 1: exact word exists
        if (index.count(token)) {
            targetWords.push_back(token);
        } 
        // Case 2: prefix match using TST
        else {
            targetWords = {"orange", "organ", "oregon"}; // tst.prefixSearch(token);
            if (targetWords.empty()) {
                return finalResults; // no match, break early
            }
        }

        // For each possible next word (including prefix expansions)
        for (const auto& nextWord : targetWords) {
            auto nextIt = index.find(nextWord);
            if (nextIt == index.end()) continue;

            const DocPosMap& nextMap = nextIt->second;
            DocPosMap resultingFileAndPositions;

            // intersect doc by doc
            for (const auto& [file, pos1] : currentToken) {
                auto found = nextMap.find(file);
                if (found != nextMap.end()) {
                    auto newPositions = positionalIntersect(pos1, found->second);
                    if (!newPositions.empty()) {
                        resultingFileAndPositions[file] = std::move(newPositions);
                    }
                }
            }
            // store nonempty results
            if (!resultingFileAndPositions.empty()) {
                finalResults[nextWord] = resultingFileAndPositions;
            }
        }
        // If only one next word matched (normal case), move forward
        if (targetWords.size() == 1 && finalResults.count(targetWords[0])) {
            currentToken = std::move(finalResults[targetWords[0]]);
            finalResults.clear(); // reset for next iteration
        } else {
            // break early, branched into multiple possiblitys
            break;
        }
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

