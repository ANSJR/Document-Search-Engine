// UNDER DEVELOPMENT
#include "../include/PdfReader.h"

#include <memory>
#include <stdexcept>
#include <string>

std::string PdfReader::readText(const std::string& filePath) {
    // std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(filePath));
    // if (!doc) throw std::runtime_error("Failed to open PDF file: " + filePath);

    // std::string text;
    // for (int i = 0; i < doc->pages(); ++i) {
    //     std::unique_ptr<poppler::page> page(doc->create_page(i));
    //     if (page) text += page->text().to_latin1() + "\n";
    // }
    return filePath;
}