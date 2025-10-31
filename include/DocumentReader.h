/*
 * DocumentReader.h
 * 
 * Abstract base class that defines a uniform interface for reading text
 * from different document formats (.txt, .pdf, etc.). Concrete subclasses
 * implement the readText() method for their specific file types.
 */

#ifndef DOCUMENT_READER_H
#define DOCUMENT_READER_H
#include <string>

class DocumentReader {
public:
    virtual std::string readText(const std::string& filePath) = 0;
    virtual ~DocumentReader() = default;
};
#endif