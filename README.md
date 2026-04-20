# Document Search Engine

A C++ document search engine for indexing and searching local text-based files, designed as a backend component that can later be exposed through a local API and can be integrated into other applications.

## Current Status

**Status: v1.0 (Initial Release)**

The document search engine is fully functional end-to-end and actively evolving. Core indexing, ranked search, prefix lookup, and incremental file updates are all implemented and accessible via a local HTTP API built with Crow. Internally, the engine uses an inverted index combined with a Ternary Search Tree for efficient lookup.

Further work will focus on performance improvements, expanded file support, and API refinement/additions.

## Quick Start

```bash
make
make run
```
Once the server is running, API can be called at:
http://localhost:2323

---

### Features
- Indexes local `.txt` and `.md` files from a directory
- Tokenizes and normalizes text for efficient search
- Builds an inverted index with positional metadata
- Supports ranked retrieval using TF-IDF-style scoring
- Prefix-based lookup via a Ternary Search Tree (TST)
- Incremental file indexing and deletion
- Local HTTP API powered by Crow
- Exposes a set of HTTP API endpoints (see below)
- Builds and runs from the command line

### Planned

- Improving result formatting of JSON search results for integration into future apps
- Cleaning up architecture between engine, search, indexing, and API layers
- Snippet/context extraction via byte offsets
- Multithreaded parallel index construction
- Integration into a future application

---

## Project Goal

The long-term goal of this project is to become a reusable local search backend that can be attached to other software or used stand alone as a highly efficient tool with a strong emphasis on performance and efficiency.

---

## Current Search Data Model

The engine stores positional information for matched terms, including:
- terms and their associated file
- token position
- byte offset
- token length

---

## Tech Stack

- C++17
- Crow HTTP framework
- Inverted index
- Ternary Search Tree (TST)
- TF-IDF-style ranking

---

## Architecture Overview

- **Tokenizer** handles normalization and token extraction
- **Indexer** builds and updates the inverted index
- **Searcher** performs ranked retrieval and query evaluation
- **TernarySearchTree** supports efficient prefix-based lookup
- **API server** exposes indexing and search functionality over HTTP

---

### Current endpoints
**GET /health**  
Confirms that the API server is running.

**GET /search?q=<query>**  
Executes a search query and returns ranked results.

**GET /indexHealth**  
Returns a short statistic about the current index.

**POST /initialIndex?path=<directory>**  
Builds the initial index from a directory of files.

**POST /index/file?path=<file>**  
Indexes or reindexes a file, updating affected terms.

**DELETE /index/file?path=<file>**  
Removes a file and cleans up unused terms from the index.


### Example
```bash
curl http://localhost:2323/health
curl -X POST "http://localhost:2323/initialIndex?path=data"
curl http://localhost:2323/indexHealth
curl "http://localhost:2323/search?q=birthday"
curl -X POST "http://localhost:2323/index/file?path=dataTemp/BBcase.txt"
curl http://localhost:2323/indexHealth
curl "http://localhost:2323/search?q=birthday"
curl -X DELETE "http://localhost:2323/index/file?path=dataTemp/BBcase.txt"
curl http://localhost:2323/indexHealth
```
{
  "query": "birthday",
  "count": 1,
  "results": [
    {
      "file": "data/WarAndPeace.txt",
      "score": 0,
      "termMatches": [
        {
          "term": "birthday",
          "occurrences": [
            {
              "tokenPos": 443617,
              "byteOffset": 2522350,
              "length": 8
            },
            {
              "tokenPos": 443651,
              "byteOffset": 2522568,
              "length": 8
            },
            {
              "tokenPos": 443854,
              "byteOffset": 2523748,
              "length": 8
            }
          ]
        }
      ]
    }
  ]
}
