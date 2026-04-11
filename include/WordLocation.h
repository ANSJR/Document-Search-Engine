#ifndef WORDLOCATION_H
#define WORDLOCATION_H
#include <cstddef>
#include <fstream>


struct WordLocation {
    size_t tokenPos;
    std::streampos byteOffset;
};

#endif