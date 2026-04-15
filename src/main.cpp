/*
 * main.cpp
 * 
 * Entry point for the Document Search Engine.
 * Creates a TxtReader, builds an index for sample text files,
 * and demonstrates the basic indexing pipeline.
 */

#include "../include/Engine.h"
#include <iomanip>
#include <iostream>
#include <filesystem>

int main() {
    Engine engine;

    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator("data")) {
        if (entry.is_regular_file() && (entry.path().extension() == ".txt" || entry.path().extension() == ".md")) {
            files.push_back(entry.path().string());
        }
    }

    engine.indexFiles(files);

    // Search query
    std::string query;
    std::cout << "\nEnter Query: ";
    std::getline(std::cin, query);
    std::cout << std::endl;
    const auto& queryResults = engine.search(query);
    std::cout << "EXITING CODE";
    return 0;
}
