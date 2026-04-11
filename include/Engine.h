
#include "../include/Indexer.h"
#include "../include/Searcher.h"
#include "../include/TernarySearchTree.h"

struct MatchOccurrence {
    std::string term;
    size_t tokenPos;
    std::streamoff byteOffset;
    size_t length;
};

struct SearchResult {
    std::string file;
    double score;
    std::vector<MatchOccurrence> occurrences;
};

class Engine {
private:
    TernarySearchTree tst;
    Indexer indexer;

public:
    Engine();
    void indexFiles(const std::vector<std::string>& files);
    std::vector<SearchResult> search(const std::string& query) const;
    std::vector<std::string> prefixSearch(const std::string& prefix) const;
};