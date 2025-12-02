// UNDER DEVELOPMENT

#ifndef PDF_READER_H
#define PDF_READER_H

#include "DocumentReader.h"
#include <string>

class PdfReader : public DocumentReader {
public:
    std::string readText(const std::string& filePath) override;
};

#endif