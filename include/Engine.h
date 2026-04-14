
#include "../include/Indexer.h"
#include "../include/Searcher.h"
#include "../include/TernarySearchTree.h"
#include "../include/Tokenizer.h"

struct MatchOccurrence {
    size_t tokenPos;
    std::streampos byteOffset;
    size_t length;
};

struct TermMatch {
    std::string term;
    std::vector<MatchOccurrence> occurrences;
};

struct SearchResult {
    std::string file;
    double score;
    std::vector<TermMatch> termMatches;
};

class Engine {
private:
    TernarySearchTree tst;
    Indexer indexer;
    Tokenizer tokenizer;

public:
    Engine();
    void indexFiles(const std::vector<std::string>& files);
    std::vector<SearchResult> search(const std::string& query) const;
    void printSearchResults(const std::vector<SearchResult>& results) const;
};