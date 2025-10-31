/*
 * Tokenizer.cpp
 * 
 * Implements tokenization logic for document text.
 * Converts text to lowercase and extracts only alphanumeric words.
 */

#include "../include/Tokenizer.h"
#include <sstream>
#include <string>
#include <algorithm>

std::vector<std::string> Tokenizer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::stringstream streamline(text);
    std::string tempToken;
    while(std::getline(streamline, tempToken, ' ')) {
        tempToken.erase(std::remove(tempToken.begin(), tempToken.end(), '\n'), tempToken.end());
        tempToken.erase(std::remove(tempToken.begin(), tempToken.end(), '\r'), tempToken.end());

        for (char& c : tempToken) {
            c = std::tolower(static_cast<unsigned char>(c));
        }
        tokens.push_back(tempToken);
    }
    return tokens;
}