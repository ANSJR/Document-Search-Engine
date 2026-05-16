#ifndef WORDLOCATION_H
#define WORDLOCATION_H
#include <cstddef>
#include <fstream>
#include <cstdint>

struct WordLocation {
    uint64_t  tokenPos;
    uint64_t  byteOffset;
};

#endif