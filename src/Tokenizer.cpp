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
    std::string current;
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {  // isalnum return true if letter or num
            current += std::tolower(static_cast<unsigned char>(c));
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}
