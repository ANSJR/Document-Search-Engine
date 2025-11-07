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
#include "../include/TernarySearchTree.h"
#include <iostream>

int main() {
    TxtReader reader;
    Indexer indexer(reader);

    std::vector<std::string> files = {"data/doc1.txt", "data/doc2.txt"};
    TernarySearchTree tst;
    indexer.buildIndex(files, tst);

    auto index = indexer.getIndex();
    for (const auto& [word, values] : index) {
        std::cout << word << ": ";
        for (const auto& [fileName, Positions] : values){
            std::cout << fileName << " ";
            for (const auto& pos : Positions){
                std::cout << pos << ", ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl << "^^^index size : " << index.size() << std::endl;
    std::cout << "\n\nTST data : \n";
    tst.printTST();

    // Create Searcher
    Searcher searcher(index, tst);

    // Search query
    std::string query;
    // std::cout << "\nEnter Query: ";
    // std::getline(std::cin, query);

    // Perform search the 4 cases
    query = "apple";          // first word in document
    searcher.search(query);
    query = "banana";         // middle word in document
    searcher.search(query);
    query = "apple banana";   // word + word
    searcher.search(query);
    query = "ban";            // prefix search
    searcher.search(query);
    query = "apple ban";      // word + prefix
    searcher.search(query);
    query = "or man";         // prefix + prefix
    searcher.search(query);
    // IMPORTANT prefix + prefix is fickely depending on the words in the documents
    std::cout << std::endl;
    

    return 0;
}
