// UNDER DEVELOPMENT
/*
 * DocumentReaderFactory.cpp
 * 
 * The implementation will check the file extension and create the appropriate object.
 * Using std::unique_ptr is the modern C++ approach for managing object lifetimes to 
 * prevent memory leaks
 * 
 */

#include "../include/DocumentReaderFactory.h"
#include "../include/TxtReader.h"
#include "../include/PdfReader.h"
#include <algorithm>
#include <stdexcept>

std::unique_ptr<DocumentReader> DocumentReaderFactory::createReader(const std::string& filePath) {
    auto pos = filePath.find_last_of('.');
    if (pos == std::string::npos)
        throw std::invalid_argument("File has no extension.");

    std::string ext = filePath.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "txt")
        return std::make_unique<TxtReader>();
    else if (ext == "pdf")
        return std::make_unique<PdfReader>();
    else
        throw std::invalid_argument("Unsupported file type: " + ext);
}