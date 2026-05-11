#include <../include/Searcher.h>



Searcher::Searcher(const std::unordered_map<std::string,
            std::unordered_map<std::filesystem::path, std::vector<WordLocation>>>& idx,
            const TernarySearchTree& tst) : index(idx), tst(tst) {}

std::unordered_map<std::string, std::unordered_map<std::filesystem::path, std::vector<WordLocation>>> Searcher::search(const std::string& query) {
    std::vector<std::string> queryTokens = tokenizer.simpleTokenize(query);
    if (queryTokens.empty()) return {};
    // std::cout << "TOTAL TOKENS : " << tokenizer.getTotalTokens(query) << std::endl;

    auto results = chainedPositionalIntersect(index, queryTokens);

    // std::cout << "\n\nDEBUGGING PRINT Query (" << query << ") : \n";
    // for (const auto& [word, docMap] : results) {
    //     for (const auto& [file, positions] : docMap) {
    //         std::cout << "  " << file << " -> ";
    //         for (const WordLocation& pos : positions) {
    //             std::cout << "(" << pos.tokenPos << "," << pos.byteOffset << ") ";
    //         }
    //         std::cout << "\n";
    //     }
    // }
    // std::cout << std::endl;
    return results;
}
// Intersect positions of two tokens within the same document
std::vector<WordLocation> Searcher::positionalIntersect(const std::vector<WordLocation>& pos1, const std::vector<WordLocation>& pos2) {
    std::vector<WordLocation> result;
    result.reserve(std::min(pos1.size(), pos2.size()));
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
std::unordered_map<std::string, std::unordered_map<std::filesystem::path, std::vector<WordLocation>>>
Searcher::chainedPositionalIntersect(const std::unordered_map<std::string, std::unordered_map<std::filesystem::path, std::vector<WordLocation>>>& index, const std::vector<std::string>& queryTokens
) {
    using DocPosMap = std::unordered_map<std::filesystem::path, std::vector<WordLocation>>;
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
    const DocPosMap* currentTokenPtr = nullptr;
    DocPosMap currentComputed;
    std::string currentWord;
    bool firstWordSet = false;
    for (const auto& word : firstMatches) {
        auto it = index.find(word);
        if (it != index.end()) {
            currentTokenPtr = &it->second;
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
            for (const auto& [file, pos1] : *currentTokenPtr) {
                auto found = nextMap.find(file);
                if (found != nextMap.end()) {
                    auto newPositions = positionalIntersect(pos1, found->second);
                    if (!newPositions.empty()) {
                        resultingFileAndPositions[file] = std::move(newPositions);
                    }
                }
            }
            if (!resultingFileAndPositions.empty()) {
                nextResults.emplace(nextWord, std::move(resultingFileAndPositions));
            }
        }
        if (nextResults.empty()) return {};
        if (targetWords.size() == 1) {
            currentWord = targetWords[0];
            currentComputed = std::move(nextResults.begin()->second);
            currentTokenPtr = &currentComputed;
        } else {
            finalResults = std::move(nextResults);
            break;
        }
    }
    if (finalResults.empty()) {
        if (currentTokenPtr == &currentComputed) {
            return {{currentWord, std::move(currentComputed)}};
        }
        return {{currentWord, *currentTokenPtr}};
    }
    return finalResults;
}

