/*
 * Tokenizer.cpp
 * 
 * Implements tokenization logic for document text.
 * Converts text to lowercase and extracts only alphanumeric words.
 */

#include "../include/Tokenizer.h"

std::vector<std::string> Tokenizer::simpleTokenize(const std::string& text) {
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
std::size_t Tokenizer::getTotalTokens(const std::string& text) {
    std::string current;
    size_t TotalTokens = 1;
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
            TotalTokens++;
            current.clear();
        }
    }
    return TotalTokens;
}
std::vector<std::pair<std::string, WordLocation>> Tokenizer::tokenize(const std::string& text) {
    std::vector<std::pair<std::string, WordLocation>> tokens;
    std::string current;
    std::streampos wordStart = -1;
    size_t tokenPos = 0;

    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        size_t consumedBytes = 1;
        bool isTokenChar = false;
        // normalize curly apostrophe → ASCII '
        if ((unsigned char)c == 0xE2 &&
            i + 2 < text.size() &&
            (unsigned char)text[i+1] == 0x80 &&
            ((unsigned char)text[i+2] == 0x99 || (unsigned char)text[i+2] == 0x98)) {
            c = '\'';
            consumedBytes = 3;
            isTokenChar = true;
        }
        else if (std::isalnum(static_cast<unsigned char>(c)) || c == '\'' || c == '-') {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            isTokenChar = true;
        }
        if (isTokenChar) {
            if (current.empty()) {
                wordStart = static_cast<std::streampos>(i);
            }
            current += c;
            if (consumedBytes == 3) i += 2;
        }
        else if (!current.empty()) {
            tokens.push_back({current, WordLocation{tokenPos, wordStart}});
            current.clear();
            wordStart = -1;
            tokenPos++;
        }
    }
    if (!current.empty()) {
        tokens.push_back({current, WordLocation{tokenPos, wordStart}});
    }

    return tokens;
}