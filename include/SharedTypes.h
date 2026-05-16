#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>
#include "WordLocation.h"

struct FileMetadata {
    size_t tokenCount;
    std::unordered_set<std::string> uniqueTerms;
    uint64_t generation = 0;
};

using LocalIndex = std::unordered_map<std::string, std::vector<WordLocation>>;
using LocalFileToTerms = FileMetadata;

struct PartialResult {
    std::filesystem::path filePath;
    LocalIndex localIndex;
    LocalFileToTerms localFileToTerms;
};

#endif