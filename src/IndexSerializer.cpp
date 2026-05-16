
#include "../include/IndexSerializer.h"
#include "../include/Indexer.h"
#include "../include/TernarySearchTree.h"
#include <future>
#include <stdexcept>

static_assert(std::is_trivially_copyable_v<WordLocation>);
template<typename T>
void writeBinary(std::ofstream& out, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>, "Binary serialization requires POD types");
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));

}
template<typename T>
T readBinary(std::ifstream& in) {
    static_assert(std::is_trivially_copyable_v<T>, "Binary deserialization requires POD types");
    T value;
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in) {
        throw std::runtime_error("Failed binary read");
    }
    return value;
}
/*
[pathLen]
[absolute path] ----
[tokenCount] 13
[generation] 1
[totalTerms] 8

repeat:
    [termLen] 3
    [term chars] ate
    [WordLocation count] 2
    [WordLocation array] 
*/
bool IndexSerializer::save(const Indexer& indexer, const std::filesystem::path& currFile, const std::filesystem::path& dataFolder) {
    std::ofstream out(dataFolder, std::ios::binary);
    if (!out) {
        return false;
    }

    auto fileIt = indexer.fileToTerms.find(currFile);
    if (fileIt == indexer.fileToTerms.end()) {
        return false;
    }

    const FileMetadata& metadata = fileIt->second;

    // Filename
    std::string originalPath = currFile.string();
    uint64_t pathLen = originalPath.size();
    writeBinary(out, pathLen);
    out.write(originalPath.data(), pathLen);

    // tokenCount
    writeBinary(out, metadata.tokenCount);
    // generation
    writeBinary(out, metadata.generation);
    // totalTerms
    uint64_t totalTerms = metadata.uniqueTerms.size();
    writeBinary(out, totalTerms);
    uint64_t writtenTerms = 0;

    for (const auto& term : metadata.uniqueTerms) {
        auto indexIt = indexer.index.find(term);
        if (indexIt == indexer.index.end()) continue;

        auto postingIt = indexIt->second.find(currFile);
        if (postingIt == indexIt->second.end()) continue;
        writtenTerms++;
        const auto& locations = postingIt->second;

        // termLen , term chars
        uint64_t termLen = term.size();
        writeBinary(out, termLen);
        out.write(term.data(), termLen);

        // WordLocation count, WordLocation array
        uint64_t locCount = locations.size();
        writeBinary(out, locCount);
        out.write(reinterpret_cast<const char*>(locations.data()), locCount * sizeof(WordLocation));
    }
    assert(writtenTerms == totalTerms);
    return out.good();
}
