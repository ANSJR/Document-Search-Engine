/*
 * TxtReader.h
 * 
 * Implements the DocumentReader interface for plain text files.
 * Uses std::ifstream to read file contents and return them as a string.
 */

#ifndef TXT_READER_H
#define TXT_READER_H

#include "DocumentReader.h"
#include <string>

class TxtReader : public DocumentReader {
public:
    std::string readText(const std::string& filePath) override;
    ~TxtReader() override = default;
};

#endif