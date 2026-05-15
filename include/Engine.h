#ifndef ENGINE_H
#define ENGINE_H
#include "../include/SearchTypes.h"
#include "../include/Indexer.h"
#include "../include/Searcher.h"
#include "../include/TernarySearchTree.h"
#include "../include/Tokenizer.h"
#include <filesystem>
#include <future>
#include <queue>

struct DirtyJob {
    std::filesystem::path file;
    uint64_t generation;
};

class Engine {
private:
    TernarySearchTree tst;
    Indexer indexer;

    std::thread serializerWorker;
    std::queue<DirtyJob> dirtyQueue;
    std::mutex dirtyMutex;
    std::condition_variable dirtyCV;
    bool stopWorker = false;

    void serializerLoop();

public:
    Engine();
    void indexFiles(const std::vector<std::filesystem::path>& files);
    void indexFile(const std::filesystem::path& filePath);
    void deleteTermFromFile(const std::filesystem::path& filePath);
    std::vector<SearchResult> search(const std::string& query) const;
    int getTotalIndexTerms() const;
    int getTotalTreeTerms() const;
    ~Engine();
};
#endif