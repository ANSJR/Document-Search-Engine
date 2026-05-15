
#include "../include/IndexSerializer.h"
#include "../include/Indexer.h"
#include "../include/TernarySearchTree.h"
#include <filesystem>


bool IndexSerializer::save(const Indexer& indexer, const std::filesystem::path& outputFile) {
    return true;
}
bool IndexSerializer::load(Indexer& indexer, TernarySearchTree& tst, const std::filesystem::path& inputFile) {
    return true;
}