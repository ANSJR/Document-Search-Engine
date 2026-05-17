
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
bool IndexSerializer::load(Indexer& indexer, TernarySearchTree& tst, const std::filesystem::path& dataFolder) {
    std::cout << "LOADING INDEXBIN\n";
    try {
        std::vector<std::filesystem::path> files;
        // collect all .bin files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dataFolder)) {
            if (entry.is_regular_file()) {
                auto normalized = std::filesystem::weakly_canonical(entry.path());
                auto ext = normalized.extension();
                if (ext == ".bin") {
                    files.push_back(normalized);
                }
            }
        }
        if (files.empty()) return true;

        loadIndex(indexer, tst, files);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Load failed: " << e.what() << '\n';
        return false;
    }
}

PartialResult IndexSerializer::partialLoadIndexThreadWorkers(const std::filesystem::path& filePath) {
    std::ifstream in(filePath, std::ios::binary);
    if (!in) throw std::runtime_error("Failed reading term");
    PartialResult result;

    uint64_t pathLen = readBinary<uint64_t>(in);
    std::string pathStr(pathLen, '\0');
    in.read(pathStr.data(), pathLen);
    if (!in) throw std::runtime_error("Failed reading path");
    std::filesystem::path originalPath = pathStr;
    result.filePath = originalPath;

    result.localFileToTerms.tokenCount = readBinary<uint64_t>(in);
    result.localFileToTerms.generation = readBinary<uint64_t>(in);
    uint64_t totalTerms = readBinary<uint64_t>(in);

    for (uint64_t i = 0; i < totalTerms; i++) {
        uint64_t termLen = readBinary<uint64_t>(in);

        std::string term(termLen, '\0');
        in.read(term.data(), termLen);
        if (!in) throw std::runtime_error("Failed reading term");
        result.localFileToTerms.uniqueTerms.insert(term);

        uint64_t locCount = readBinary<uint64_t>(in);
        std::vector<WordLocation> locations(locCount);
        in.read(reinterpret_cast<char*>(locations.data()), locCount * sizeof(WordLocation));
        if (!in) throw std::runtime_error("Failed reading term");
        result.localIndex[term] = std::move(locations);
    }
    return result;

}
void IndexSerializer::mergePartialLoadIndexThreadWorkers(Indexer& indexer, PartialResult&& partial, TernarySearchTree& tst) {
    
    for (auto& [token, location] : partial.localIndex) {
        auto [it, inserted] = indexer.index.try_emplace(token);
        if (inserted) {tst.insert(token);}
        // it->second is unordered_map<path, vector<WordLocation>>
        it->second[partial.filePath] = std::move(location);
    }
    indexer.fileToTerms[partial.filePath] = std::move(partial.localFileToTerms);
    indexer.totalTokensInIndex += indexer.fileToTerms[partial.filePath].tokenCount;
}
void IndexSerializer::loadIndex(Indexer& indexer, TernarySearchTree& tst, const std::vector<std::filesystem::path>& files) {
    const size_t maxThreads = std::max(1u, std::thread::hardware_concurrency());

    for (size_t i = 0; i < files.size(); i += maxThreads) {
        std::vector<std::future<PartialResult>> futures;
        size_t end = std::min(i + maxThreads, files.size());
        futures.reserve(end - i);
        for (size_t j = i; j < end; j++) {
            futures.push_back(std::async(std::launch::async, &IndexSerializer::partialLoadIndexThreadWorkers, files[j]));
        }
        for (auto& future : futures) {
            IndexSerializer::mergePartialLoadIndexThreadWorkers(indexer, std::move(future.get()),tst);
        }
    }
}