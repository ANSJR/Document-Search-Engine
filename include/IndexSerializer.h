#ifndef INDEX_SERIALIZER_H
#define INDEX_SERIALIZER_H

#include "SharedTypes.h"
#include <filesystem>
#include <cstdint>
#include <cassert>

class Indexer;
class TernarySearchTree;

class IndexSerializer {
private:
    static PartialResult partialLoadIndexThreadWorkers(const std::filesystem::path& files);
    static void mergePartialLoadIndexThreadWorkers(Indexer& indexer, PartialResult&& partial, TernarySearchTree& tst);
    static void loadIndex(Indexer& indexer, TernarySearchTree& tst, const std::vector<std::filesystem::path>& files);

public:
    static bool save(const Indexer& indexer, const std::filesystem::path& currFile, const std::filesystem::path& dataFolder);

    static bool load(Indexer& indexer, TernarySearchTree& tst, const std::filesystem::path& dataFolder);
};

#endif