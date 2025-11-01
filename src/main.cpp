/*
 * main.cpp
 * 
 * Entry point for the Document Search Engine.
 * Creates a TxtReader, builds an index for sample text files,
 * and demonstrates the basic indexing pipeline.
 */

#include "../include/TxtReader.h"
#include "../include/Indexer.h"
#include "../include/Searcher.h"
#include <iostream>

int main() {
    TxtReader reader;
    Indexer indexer(reader);

    std::vector<std::string> files = {"data/doc1.txt", "data/doc2.txt"};
    indexer.buildIndex(files);

    auto index = indexer.getIndex();
    // for (const auto& [word, values] : index) {
    //     std::cout << word << ": ";
    //     for (const auto& [fileName, Positions] : values){
    //         std::cout << fileName << " ";
    //         for (const auto& pos : Positions){
    //             std::cout << pos << ", ";
    //         }
    //     }
    //     std::cout << "\n";
    // }
    // std::cout << std::endl << index.size();


    // Optional TST pointer (nullptr for now)
    TernarySearchTree* tst = nullptr;

    // Create Searcher
    Searcher searcher(index, tst);

    // Search query
    std::string query = "those who wrote that we might read";
    searcher.search(query);

    return 0;
}
