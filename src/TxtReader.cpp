/*
 * TxtReader.cpp
 * 
 * Contains the implementation of TxtReader::readText(), which reads
 * text data from .txt files using standard C++ file streams.
 */

#include "../include/TxtReader.h"
#include <fstream>
#include <sstream>


std::string TxtReader::readText(const std::string& filePath) {
    std::ifstream file(filePath);
    std::ostringstream buffer; // in-memory stream, dynamically sized std::string that is stored internally
    buffer << file.rdbuf(); // reads data from source buffer and writes to internally stored string
    return buffer.str(); // buffer.str returns finalized internally stored string
}