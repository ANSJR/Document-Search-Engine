/*
 * Tokenizer.h
 * 
 * Provides utility functions to split document text into lowercase,
 * alphanumeric tokens for indexing and search.
 * Removes punctuation and normalizes case.
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "WordLocation.h"
#include <sstream>
#include <string>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>

class Tokenizer {
public:
    std::vector<std::string> simpleTokenize(const std::string& text);
    std::size_t getTotalTokens(const std::string& text);
    std::string isolateLastToken(const std::string text) const;
    std::vector<std::pair<std::string, WordLocation>> tokenize(const std::string& text);
};

#endif