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
    for (int i = 0; i < text.size(); i++) {
        char c = text[i];

        // converting single quotation mark to ascii
        if ((unsigned char)c == 0xE2 && i + 2 < text.size() && (unsigned char)text[i+1] == 0x80 &&
        ((unsigned char)text[i+2] == 0x99 || (unsigned char)text[i+2] == 0x98)) {
            c = '\'';   // normalize to ASCII apostrophe
            i += 2;     // skip next 2 bytes cuz unicode is 3 bytes
        }

        if (std::isalnum(static_cast<unsigned char>(c)) || c == '\'' || c == '-') {  // isalnum return true if letter or num
            current += std::tolower(static_cast<unsigned char>(c));
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}
