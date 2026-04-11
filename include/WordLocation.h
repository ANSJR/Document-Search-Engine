#ifndef WORDLOCATION_H
#define WORDLOCATION_H


struct WordLocation {
    size_t tokenPos;
    std::streampos byteOffset;
    size_t length;
};

#endif