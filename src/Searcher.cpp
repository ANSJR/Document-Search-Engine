

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

    // Print 
    std::cout << "PRINT : ";
    for (const auto& [word, positions] : results) {
        std::cout << word << ": ";
        for (size_t pos : positions) {
            std::cout << pos << " ";
        }
        std::cout << "\n";
    }

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
std::unordered_map<std::string, std::vector<size_t>> Searcher::chainedPositionalIntersect(const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<size_t>>>& index,
    const std::vector<std::string>& queryTokens) {

    if (queryTokens.empty()) return {};

    auto token = index.find(queryTokens[0]);
    if (token == index.end()) return {};

    // Creates current with all files and their pos of first token of query from index
    std::unordered_map<std::string, std::vector<size_t>> currentToken = token->second;

    // Iterate through next tokens in query
    for (size_t i = 1; i < queryTokens.size(); ++i) {
        const std::string& token = queryTokens[i];
        std::unordered_map<std::string, std::vector<size_t>> mergedNext;

        // Case 1 Exact match 
        auto nextToken = index.find(token);
        if (nextToken != index.end()) {
            mergedNext = nextToken->second;
        }
        else {
            // Case 2 prefix match
            std::vector<std::string> prefixMatches = tst.prefixSearch(token);
            std::cout << "\n\n" << prefixMatches[0] << " " << prefixMatches[1] << std::endl;
            if (prefixMatches.empty()) {
                return {};
            }

            // Merge words that match prefix
            for (const auto& possibleWord : prefixMatches) {
                auto foundWord = index.find(possibleWord);
                if (foundWord != index.end()) {
                    for (const auto& [file, positions] : foundWord->second) {
                        auto& entry = mergedNext[file];
                        entry.insert(entry.end(), positions.begin(), positions.end());
                    }
                }
            }

            // Sort and deduplicate merged positions
            for (auto& [_, posVec] : mergedNext) {
                std::sort(posVec.begin(), posVec.end());
                posVec.erase(std::unique(posVec.begin(), posVec.end()), posVec.end());
            }
        }

        // Positional intersection
        std::unordered_map<std::string, std::vector<size_t>> resultingFileAndPositions;
        for (const auto& [file, positions1] : currentToken) {
            auto found = mergedNext.find(file);
            if (found != mergedNext.end()) {
                auto newPositions = positionalIntersect(positions1, found->second);
                if (!newPositions.empty()) {
                    resultingFileAndPositions[file] = newPositions;
                }
            }
        }
        currentToken = std::move(resultingFileAndPositions);
        if (currentToken.empty()) break;
    }

    return currentToken;
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


std::map<std::string, double> Searcher::computeScores( const std::vector<std::string>& expandedqueryTokens) const {
        for(const auto& token : expandedqueryTokens) {

        }
        return {};
    }

