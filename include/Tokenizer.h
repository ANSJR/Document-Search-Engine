/*
 * Tokenizer.h
 * 
 * Provides utility functions to split document text into lowercase,
 * alphanumeric tokens for indexing and search.
 * Removes punctuation and normalizes case.
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <vector>
#include <string>

class Tokenizer {
public:
    std::vector<std::string> tokenize(const std::string& text);
};

#endif