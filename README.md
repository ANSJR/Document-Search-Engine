# Document Search Engine

A C++ document search engine for indexing and searching local text-based files, designed as a backend component that can later be exposed through a local API and can be integrated into other applications.


## Current Status

**Status: v1.0 (Initial Release)**

The document search engine is fully functional end-to-end and actively evolving. Core indexing, ranked search, prefix lookup, and incremental file updates are all implemented and accessible via a local HTTP API built with Crow. Internally, the engine uses an inverted index combined with a Ternary Search Tree for efficient lookup.

Further work will focus on performance improvements, expanded file support, and API refinement/additions.


## Architecture

The search engine is divided into several core components:

- **Document Parser**  
  Reads text files, tokenizes content, removes punctuation, and normalizes terms.

- **Inverted Index**  
  Maps terms to posting lists containing:
  - document frequency
  - term frequency
  - positional metadata

- **Ternary Search Tree (TST)**  
  Stores indexed terms for efficient prefix-based lookup and autocomplete-style traversal.

- **Ranking Engine**  
  Uses BM25 scoring to rank documents by relevance.

- **Search API**  
  Exposes query functionality through a lightweight HTTP interface using Crow.


## Tech Stack

- C++17
- Crow HTTP framework
- Inverted index
- Ternary Search Tree (TST)
- BM25 style ranking


### Features
- Indexes local `.txt` and `.md` files from a directory
- Tokenizes and normalizes text for efficient search
- Builds an inverted index with positional metadata
- Supports ranked retrieval using BM25 ranking model
- Prefix-based lookup via a Ternary Search Tree (TST)
- Incremental file indexing and deletion
- Local HTTP API powered by Crow
- Exposes a set of HTTP API endpoints (see below)
- Builds and runs from the command line


## Project Goal

The long-term goal of this project is to become a reusable local search backend that can be attached to other software or used stand alone as a highly efficient tool with a strong emphasis on performance and efficiency.


## Current Search Data Model

The engine stores positional information for matched terms, including:
- terms and their associated file
- token position
- byte offset
- token length

Documents -> Tokenizer -> Inverted Index + TST -> BM25 Ranking -> Query Results

## Why a Ternary Search Tree?

A Ternary Search Tree (TST) was chosen over a standard trie or hash map because it provides a balance between:

- memory efficiency
- prefix-search performance
- ordered traversal capability

### Comparison

| Structure | Advantages | Disadvantages |
|---|---|---|
| Hash Map | Fast exact lookup | No prefix search |
| Trie | Fast prefix queries | High memory usage |
| TST | Prefix support with lower memory overhead | Slightly slower than trie |

The TST enables efficient:
- autocomplete
- partial matching
- lexicographical traversal

while using less memory than a traditional trie.


## BM25 Ranking

Document relevance is ranked using the BM25 scoring algorithm.

The score for a term is computed using:

$$
BM25(q,d) = IDF(q) * ( f(q,d)*(k1+1) / ( f(q,d) + k1*(1-b+b*|d|/avgdl) ) )
$$

Where:

- `f(q,d)` = frequency of term in document
- `|d|` = document length
- `avgdl` = average document length
- `k1` and `b` = tuning constants

BM25 was selected because it:

- handles term frequency saturation
- normalizes document length
- performs well in real-world information retrieval systems

## Complexity Analysis

| Operation | Complexity | Notes |
|---|---|---|
| Exact term lookup | **O(1)** average | Hash map lookup for full-word queries |
| Prefix autocomplete | **O(P + K)** average | Traverse prefix + explore matching subtree |
| Tokenization | **O(N)** | Single pass through document text |
| Document indexing | **O(T)** average | Processes every token once |
| TST insertion | **O(L)** average | Depends on word length and tree shape |
| Phrase search | **O(a + b)** | Positional posting-list intersection |
| BM25 score calculation | **O(1)** per doc-term pair | Constant-time arithmetic |
| Result ranking | **O(R log R)** | Sorting matched results by score |
| Remove indexed document | **O(U × deleteCost)** | Removes file references from postings |
| Full TST traversal | **O(M)** | Visits every node in tree |

### Symbols / Keys

- `N` = number of characters in input text
- `T` = total tokens in a document
- `U` = unique terms in a document
- `L` = term length
- `P` = prefix length
- `K` = number of matched subtree nodes/terms
- `M` = total TST nodes
- `R` = ranked result count
- `a, b` = posting list sizes for phrase intersections


### Notes

- Exact searches use hash maps for average constant-time lookups.
- Prefix searches are powered by a Ternary Search Tree (TST).
- Phrase queries rely on positional posting-list intersections.
- BM25 scoring itself is constant time, but the final ranking requires sorting results.

## Quick Start

```bash
make
make run
```
Once the server is running, API can be called at:
http://localhost:2323


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
curl -X POST "http://localhost:2323/initialIndex?path=GutenbergText"
```
{
  "query": "warr",
  "count": 2,
  "results": [
    {
      "file": "data/WarAndPeace.txt",
      "score": 0.321661,
      "termCount": 6,
      "termMatches": [
        {
          "term": "warriors",
          "occurrences": [
            {
              "tokenPos": 313176,
              "byteOffset": 1779894,
              "length": 8
            },
            {
              "tokenPos": 313923,
              "byteOffset": 1784045,
              "length": 8
            },
            {
              "tokenPos": 408580,
              "byteOffset": 2322679,
              "length": 8
            },
            {
              "tokenPos": 485112,
              "byteOffset": 2757875,
              "length": 8
            }
          ]
        },
        {
          "term": "warrior",
          "occurrences": [
            {
              "tokenPos": 44214,
              "byteOffset": 254050,
              "length": 7
            },
            {
              "tokenPos": 108457,
              "byteOffset": 622232,
              "length": 7
            },
            {
              "tokenPos": 317231,
              "byteOffset": 1802457,
              "length": 7
            },
            {
              "tokenPos": 317836,
              "byteOffset": 1805963,
              "length": 7
            },
            {
              "tokenPos": 329723,
              "byteOffset": 1873862,
              "length": 7
            },
            {
              "tokenPos": 401915,
              "byteOffset": 2285158,
              "length": 7
            }
          ]
        },
        {
          "term": "warranty",
          "occurrences": [
            {
              "tokenPos": 577606,
              "byteOffset": 3286073,
              "length": 8
            },
            {
              "tokenPos": 577677,
              "byteOffset": 3286548,
              "length": 8
            }
          ]
        },
        {
          "term": "warranties",
          "occurrences": [
            {
              "tokenPos": 577905,
              "byteOffset": 3287825,
              "length": 10
            },
            {
              "tokenPos": 577917,
              "byteOffset": 3287898,
              "length": 10
            },
            {
              "tokenPos": 577937,
              "byteOffset": 3288020,
              "length": 10
            }
          ]
        },
        {
          "term": "warrants",
          "occurrences": [
            {
              "tokenPos": 446331,
              "byteOffset": 2538063,
              "length": 8
            }
          ]
        },
        {
          "term": "warrant",
          "occurrences": [
            {
              "tokenPos": 26462,
              "byteOffset": 153559,
              "length": 7
            }
          ]
        }
      ]
    },
    {
      "file": "dataTemp/BBcase.txt",
      "score": 0.155205,
      "termCount": 1,
      "termMatches": [
        {
          "term": "warriors",
          "occurrences": [
            {
              "tokenPos": 44501,
              "byteOffset": 253483,
              "length": 8
            }
          ]
        }
      ]
    }
  ]
}