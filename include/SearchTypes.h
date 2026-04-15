#ifndef SEARCHTYPES_H
#define SEARCHTYPES_H
#include <string>
#include <vector>
#include <cstddef>

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
#endif