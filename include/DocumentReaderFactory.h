


#ifndef DOCUMENTREADERFACTORY_H
#define DOCUMENTREADERFACTORY_H

#include "DocumentReader.h"
#include "TxtReader.h"
// #include "PdfReader.h"
#include <string>
#include <memory> // For std::unique_ptr

class DocumentReaderFactory {
public:
    // This is the factory method that returns a unique_ptr to a DocumentReader
    static std::unique_ptr<DocumentReader> createReader(const std::string& filePath);
};

#endif