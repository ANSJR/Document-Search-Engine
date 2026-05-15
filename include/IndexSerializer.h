#ifndef INDEX_SERIALIZER_H
#define INDEX_SERIALIZER_H


#include <filesystem>

class Indexer;
class TernarySearchTree;


class IndexSerializer {
public:
    static bool save(const Indexer& indexer, const std::filesystem::path& outputFile);

    static bool load(Indexer& indexer, TernarySearchTree& tst, const std::filesystem::path& inputFile);
};

#endif