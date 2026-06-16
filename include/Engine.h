#ifndef ENGINE_H
#define ENGINE_H
#include "../include/SearchTypes.h"
#include "../include/Indexer.h"
#include "../include/Searcher.h"
#include "../include/TernarySearchTree.h"
#include "../include/Tokenizer.h"
#include "crow.h"
#include <filesystem>
#include <future>
#include <queue>
#include <shared_mutex>

struct EngineConfig {
    std::filesystem::path indexBin = "indexBin/";
};
struct DirtyJob {
    std::filesystem::path file;
    uint64_t generation;
};

class Engine {
private:
    EngineConfig config;
    TernarySearchTree tst;
    Indexer indexer;

    std::thread serializerWorker;
    std::queue<DirtyJob> dirtyQueue;
    std::mutex dirtyMutex;
    std::mutex pathMutex;
    // engineMutex is a reader/writer lock protecting shared index structures from concurrent search, indexing, and serialization access.
    mutable std::shared_mutex engineMutex; 
    std::condition_variable dirtyCV;
    bool stopWorker = false;

    void serializerLoop();

public:
    Engine();
    bool saveConfig(const std::filesystem::path& configPath) const;
    bool loadConfig(const std::filesystem::path& configPath);
    void loadSerializedIndex();
    void indexFiles(const std::vector<std::filesystem::path>& files);
    void indexFile(const std::filesystem::path& filePath);
    void deleteFileFromIndex(const std::filesystem::path& filePath);
    bool modifyIndexBinPath(const std::filesystem::path& filePath);
    std::vector<SearchResult> search(const std::string& query) const;
    int getTotalIndexTerms() const;
    int getTotalTreeTerms() const;
    ~Engine();
};
#endif