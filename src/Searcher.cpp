

#include <../include/Searcher.h>
#include <iostream>
#include <cmath>


Searcher::Searcher(const std::unordered_map<std::string,
            std::unordered_map<std::string, std::vector<size_t>>>& idx,
            const TernarySearchTree* tst/*= nullptr*/) : index(idx) {}

std::vector<std::pair<std::string, double>> Searcher::search(const std::string& query) {
    std::vector<std::string> queryTokens = tokenizer.tokenize(query);
    if (queryTokens.empty()) return {};

    auto results = chainedPositionalIntersect(index, queryTokens);

    // Print 
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

    // For each next word, intersect positions doc-by-doc
    for (int i = 1; i < queryTokens.size(); i++) {
        // Creates nextToken with all files and their pos of next token of query from index
        auto nextToken = index.find(queryTokens[i]);

        if (nextToken == index.end()) {
            if (i + 1 != queryTokens.size()) return {};  // if next token not found return nothing ERROR FOR PREFIX
///////////////////bool prefixFound = tst.search();
        }


        std::unordered_map<std::string, std::vector<size_t>> resultingFileAndPositions;

        for (const auto& [file, positions1] : currentToken) {
            // If theres a matching file in nextToken to current
            auto found = nextToken->second.find(file);
            if (found != nextToken->second.end()) {
                // Gets Positional data of nextToken
                auto positions2 = found->second;
                // Finds intersection of Positional data of both current and nextToken
                auto newPositions = positionalIntersect(positions1, positions2);
                // If this file has matching Positional data for both
                if (!newPositions.empty()) {
                    // Hold data to use as next current token since this data had matched the prev current token and next token
                    resultingFileAndPositions[file] = newPositions;
                }
            }
        }
        // Will take replace prev current to check the next token in loop
        currentToken = std::move(resultingFileAndPositions);
    }

    return currentToken;  // Only docs where full sequence occurs survive
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


// added previousToken
std::map<std::string, double> Searcher::computeScores( const std::vector<std::string>& expandedqueryTokens) const {
        for(const auto& token : expandedqueryTokens) {

        }
        return {};
    }

