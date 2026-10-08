
#include "../include/IndexSerializer.h"
#include "../include/Indexer.h"
#include "../include/TernarySearchTree.h"
#include <future>
#include <stdexcept>
#include <cstring>

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
bool IndexSerializer::save(const Indexer& indexer, const std::filesystem::path& currFile, const std::filesystem::path& outputFile) {
    std::ofstream out(outputFile, std::ios::binary);
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
bool IndexSerializer::load(Indexer& indexer, TernarySearchTree& tst, const std::filesystem::path& outputFile) {
    // std::cout << "LOADING INDEXBIN\n";
    try {
        std::vector<std::filesystem::path> files;
        // collect all .bin files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(outputFile)) {
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

// BinReader: walks through a block of bytes that's already in memory
// and pulls values out of it, one after another, in order.
struct BinaryReader {
    const char* cur;
    const char* end;

    BinaryReader(const std::string& buf) : cur(buf.data()), end(buf.data() + buf.size()) {}

    template <typename T>
    T read() {
        // Compile-time check: only allow "plain data" types like int, uint64_t, or simple structs.
        static_assert(std::is_trivially_copyable_v<T>, "POD only");
        // Prevent reading past end
        if (cur + sizeof(T) > end) throw std::runtime_error("Buffer underrun");

        // Copy sizeof(T) bytes from the buffer into a real variable.
        T value;
        std::memcpy(&value, cur, sizeof(T));   // memcpy is the safe way to read a POD from bytes
        cur += sizeof(T);
        return value;
    }
    // readBytes(): copy a raw chunk of n bytes into dst and move forward.
    // Used for things whose size is only known at runtime:
    //   - string characters:   r.readBytes(term.data(), termLen);
    //   - arrays of structs:   r.readBytes(locations.data(), locCount * sizeof(WordLocation));
    // dst is void* so it accepts a pointer to anything.
    void readBytes(void* dst, size_t n) {
        if (cur + n > end) throw std::runtime_error("Buffer underrun");
        std::memcpy(dst, cur, n);
        cur += n;
    }
};

PartialResult IndexSerializer::partialLoadIndexThreadWorkers(const std::filesystem::path& filePath) {
    // ONE read of the whole segment into memory (same idea as readText's rdbuf slurp)
    std::ifstream in(filePath, std::ios::binary);
    if (!in) throw std::runtime_error("Failed opening segment: " + filePath.string());
    std::string buf;
    {
        std::ostringstream ss;
        ss << in.rdbuf();
        buf = std::move(ss).str();
    }


    BinaryReader r(buf);
    PartialResult result;

    // Tedious since it use to be binary
    uint64_t pathLen = r.read<uint64_t>(); // Reads path length 
    std::string pathStr(pathLen, '\0');    // Make a string of that length
    r.readBytes(pathStr.data(), pathLen); // Reads path name
    result.filePath = std::filesystem::path(pathStr); // Stores in index

    result.localFileToTerms.tokenCount = r.read<uint64_t>(); // tokenCount (count of all tokens)
    result.localFileToTerms.generation = r.read<uint64_t>(); // generation
    uint64_t totalTerms = r.read<uint64_t>(); // total terms (not including duplicates)

    result.localFileToTerms.uniqueTerms.reserve(totalTerms);
    result.localIndex.reserve(totalTerms);

    for (uint64_t i = 0; i < totalTerms; i++) {
        uint64_t termLen = r.read<uint64_t>(); // Reads term Length
        std::string term(termLen, '\0'); // Make a string of that length
        r.readBytes(term.data(), termLen); // Reads term

        uint64_t locCount = r.read<uint64_t>(); // Reads how many locations
        std::vector<WordLocation> locations(locCount); // Make space
        r.readBytes(locations.data(), locCount * sizeof(WordLocation)); // loads locations into index

        result.localFileToTerms.uniqueTerms.insert(term);
        result.localIndex.emplace(std::move(term), std::move(locations));
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
bool IndexSerializer::deleteFile(const std::filesystem::path& sourceFile, const std::filesystem::path& outputFile) {
    std::filesystem::path binPath = outputFile / (sourceFile.filename().string() + ".bin");
    binPath = binPath.lexically_normal();

    std::error_code ec;
    bool removed = std::filesystem::remove(binPath, ec);

    if (ec) {
        std::cerr << "Failed to delete bin file: " << binPath << " — " << ec.message() << '\n';
        return false;
    }
    if (!removed) {
        std::cerr << "Bin file not found (already gone?): " << binPath << '\n';
    }
    return removed;
}