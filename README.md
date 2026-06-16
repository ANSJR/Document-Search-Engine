# Document Search Engine

A C++ document search engine for indexing and searching local text-based files, designed as a backend component that can later be exposed through a local API and can be integrated into other applications.


## Current Status

**Status: v1.0 (Initial Release)**

The document search engine is fully functional end-to-end and actively evolving. Core indexing, ranked search, prefix lookup, and incremental file updates are all implemented and accessible via a local HTTP API built with Crow. Internally, the engine uses an inverted index combined with a Ternary Search Tree for efficient lookup.

Recent work has focused heavily on performance and durability:

- Multithreaded indexing via a batched fork-join strategy (parallel parse, sequential merge)
- Asynchronous incremental index persistence with dirty-file tracking, a background serialization worker, and generation-based stale-job invalidation
- Concurrent search behind a reader–writer lock, served by a multithreaded Crow server
- A benchmark suite (Google Benchmark) covering indexing, query latency, prefix search, and scaling

Further work will focus on additional performance improvements, expanded file support, and API refinement/additions.


## Architecture

The search engine is divided into several core components:

- **Document Parser / Tokenizer**  
  Reads text files, tokenizes content, lowercases terms, keeps alphanumerics (plus `'` and `-`), and normalizes Unicode curly apostrophes to ASCII. Emits each token's position and byte offset.

- **Inverted Index**  
  Maps terms to per-file posting lists containing:
  - document frequency
  - term frequency
  - positional metadata (token position + byte offset)

  Per-file metadata (token count, unique terms, generation) is tracked alongside the index for scoring and persistence.

- **Ternary Search Tree (TST)**  
  Stores indexed terms for efficient prefix-based lookup and autocomplete-style traversal.

- **Ranking Engine**  
  Uses BM25 scoring to rank documents by relevance.

- **Concurrency Layer**  
  Parallelizes bulk indexing as a batched fork-join (parallel tokenization, sequential merge), guards the index with a `std::shared_mutex` so searches run concurrently, and serves HTTP requests on a multithreaded Crow server.

- **Persistence Layer**  
  A background serializer thread writes one binary segment per source file, driven by a dirty-file queue and generation-based stale-job invalidation, so the full index is never rewritten. Segments are loaded in parallel on startup.

- **Search API**  
  Exposes query functionality through a lightweight HTTP interface using Crow.


## Tech Stack

- C++17
- Crow HTTP framework (multithreaded server)
- Inverted index
- Ternary Search Tree (TST)
- BM25 style ranking
- C++ concurrency: `std::thread`, `std::async` / `std::future`, `std::shared_mutex`, `std::condition_variable`
- Custom binary index serialization (per-file segments)
- Google Benchmark (performance benchmarking)
- GoogleTest (unit testing)


### Features
- Indexes local `.txt` and `.md` files from a directory (recursively)
- Tokenizes and normalizes text (case-folding, Unicode apostrophe normalization) for efficient search
- Builds an inverted index with positional metadata (token position + byte offset)
- Supports ranked retrieval using the BM25 ranking model
- Prefix-based lookup via a Ternary Search Tree (TST)
- Multi-word queries via positional (phrase) intersection, with TST prefix fallback for partial terms
- Incremental single-file indexing, reindexing, and deletion
- Multithreaded (batched fork-join) document indexing
- Concurrent search via a reader–writer lock; HTTP requests served multithreaded
- Asynchronous incremental index persistence — one binary segment per file, dirty-file tracking, generation-based invalidation
- Background serialization worker for non-blocking index writes
- Parallel index load on startup
- Runtime-reconfigurable persistence directory (with automatic segment migration)
- Benchmark suite for indexing speed, query latency, prefix search, and scaling
- Local HTTP API powered by Crow
- Builds and runs from the command line


## Project Goal

The long-term goal of this project is to become a reusable local search backend that can be attached to other software or used stand alone as a highly efficient tool with a strong emphasis on performance and efficiency.


## Current Search Data Model

For each matched term the engine returns positional information, including:
- the term and its associated file
- token position
- byte offset
- match length (derived from the term)

Internally, each stored occurrence holds a token position and byte offset; the match length is taken from the term string at query time.

Documents -> Tokenizer -> Inverted Index + TST -> BM25 Ranking -> Query Results


## Query Handling

A query is tokenized, then resolved token by token:

- If a token matches an indexed term exactly, that term's postings are used.
- Otherwise the token is expanded against the **TST** via prefix search (autocomplete-style matching).
- For multi-word queries, candidate postings are combined with a **positional intersection** that requires adjacent token positions, enabling phrase-style matching.

Matched documents are then scored with BM25 and returned sorted by descending score.


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

In the current implementation `k1 = 1.5` and `b = 0.75`, and the IDF term uses the
non-negative variant `IDF = ln((N - df + 0.5) / (df + 0.5) + 1)`, where `N` is the
total number of indexed documents and `df` is the number of documents containing the term.

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

## Concurrency & Performance

The engine uses concurrency in three places:

1. **Parallel document indexing (batched fork-join).** Bulk indexing processes files
   in batches sized to `std::thread::hardware_concurrency()`. Within each batch, every
   file is read and tokenized in parallel via `std::async`, each producing an isolated
   partial index with no shared state. Partial results are then merged into the global
   inverted index and TST **sequentially**, which keeps the shared structures
   contention-free during the merge. The engine then advances to the next batch.

2. **Concurrent search (reader–writer lock).** The index is guarded by a
   `std::shared_mutex`. Searches take a shared (read) lock, so many queries can run at
   once; indexing, deletion, and reconfiguration take an exclusive (write) lock. The
   Crow HTTP server runs in multithreaded mode, so requests are served concurrently.

3. **Background serialization.** Persisting the index to disk runs on its own thread
   (see [Index Persistence](#index-persistence)) so it never blocks indexing or queries.

### Indexing strategy benchmark

Two parallel indexing strategies were implemented and measured with the
`BM_BulkIndexing` benchmark (1000 documents, 10 iterations):

| Approach | Wall Time | CPU Time |
|---|---|---|
| Thread pool (parallel file parsing) | 37,669 ms | 34,373 ms |
| **Batched parallel processing** (current) | **32,812 ms** | **29,583 ms** |

The batched fork-join approach (parallel parse, sequential merge) was adopted as the
default because it produced lower wall-clock and CPU time than an explicit per-file
thread pool, with less scheduling overhead and contention.


## Index Persistence

The index is persisted to disk incrementally in the background, so the full index is
never rewritten on each change.

> Implemented asynchronous incremental index persistence using dirty-file tracking and
> generation-based stale job invalidation to avoid full-index rewrites and minimize
> memory overhead.

How it works:

- **Per-file binary segments.** Each indexed source file is serialized to its own
  `.bin` file inside a configurable index-binary directory (`indexBin`). Indexing or
  reindexing a file rewrites only that file's segment; deleting a file removes its
  segment. The whole index is never rewritten.
- **Dirty-file queue.** Each index/reindex operation enqueues a job — the file path
  plus its current generation — onto a dirty queue. A single dedicated serializer
  thread (started when the `Engine` is constructed, drained and joined on shutdown)
  consumes the queue and writes segments asynchronously, signalled by a condition
  variable.
- **Generation-based stale-job invalidation.** Every file carries a monotonically
  increasing generation that is bumped on each reindex. Before writing a segment, the
  worker compares the job's generation against the file's current generation; if the
  file was reindexed while the job was queued, the now-stale job is skipped instead of
  writing outdated state.
- **Parallel startup load.** On startup the engine scans the `indexBin` directory and
  loads all `.bin` segments using the same batched parallel strategy as indexing
  (parallel read, sequential merge) to rebuild the inverted index and TST.
- **Runtime reconfiguration.** The persistence directory can be changed at runtime
  (via `POST /config/indexBin`, or the `indexBin` field on `POST /index/build`): the
  engine stops the serializer, migrates existing `.bin` segments to the new location,
  updates and persists `config/config.json`, then restarts the serializer.

### Binary segment format

Each `.bin` segment stores a single file's postings as packed binary. `WordLocation`
is a trivially-copyable POD, so location arrays are written and read as contiguous
blocks:

```
[pathLen][path bytes]
[tokenCount][generation][termCount]
repeat termCount times:
    [termLen][term bytes]
    [locationCount][WordLocation array]
```


## Benchmarks

A benchmark suite (built on Google Benchmark) is included to track performance and
catch regressions. It covers:

- indexing speed (bulk indexing throughput)
- query latency
- prefix search performance
- scaling with document count

Build and run the benchmarks:

```bash
make runBenchmarks
make bench FILTER=BM_BulkIndexing
```

`FILTER` is passed through to `--benchmark_filter`, so you can run a single benchmark
or a regex group. Example result:

```
BM_BulkIndexing/1000/iterations:10    Time = 32812 ms    CPU = 29583 ms
```


## Quick Start

```bash
make
make run
```
Once the server is running, the API can be called at:
http://localhost:2323

### Build targets

```bash
make                       # build the search engine (release flags)
make run                   # build and run the server
make test                  # build and run unit tests (GoogleTest)
make bench FILTER=<name>   # build and run benchmarks (Google Benchmark)
make release               # clean rebuild with release flags (-O3)
make debug                 # clean rebuild with debug flags (-O0 -g)
make clean                 # remove build artifacts
```


### Current endpoints

**GET /health**  
Confirms that the API server is running.

**GET /index/Health**  
Returns a short statistic about the current index (number of indexed terms and TST words).

**GET /search?q=&lt;query&gt;**  
Executes a search query and returns ranked results.

**POST /index/build**  
Builds (or rebuilds) the index from a directory of files.  
JSON body: `{"documentPath": "<directory>"}`.  
Optional `"indexBin": "<directory>"` sets and persists the index-binary (persistence) directory before building.

**POST /index/file**  
Indexes or reindexes a single file, updating affected terms.  
JSON body: `{"path": "<file>"}`.

**DELETE /index/file?path=&lt;file&gt;**  
Removes a file from the index, cleans up unused terms, and deletes its `.bin` segment.

**POST /config/indexBin**  
Changes the persistence (index-binary) directory at runtime and migrates existing segments.  
JSON body: `{"path": "<directory>"}`.


### Example

```bash
curl http://localhost:2323/health

# Build the initial index from the ./data directory
curl -X POST http://localhost:2323/index/build \
  -H "Content-Type: application/json" \
  -d '{"documentPath":"data"}'

# Build, and route persistence to a specific index-binary directory
curl -X POST http://localhost:2323/index/build \
  -H "Content-Type: application/json" \
  -d '{"documentPath":"data","indexBin":"indexBin"}'

# Change the persistence directory at runtime
curl -X POST http://localhost:2323/config/indexBin \
  -H "Content-Type: application/json" \
  -d '{"path":"indexBin"}'

curl http://localhost:2323/index/Health

curl "http://localhost:2323/search?q=birthday"

# Incrementally add / reindex a single file
curl -X POST http://localhost:2323/index/file \
  -H "Content-Type: application/json" \
  -d '{"path":"dataTemp/BBcase.txt"}'

curl http://localhost:2323/index/Health

curl "http://localhost:2323/search?q=birthday"

# Remove a file from the index
curl -X DELETE "http://localhost:2323/index/file?path=dataTemp/BBcase.txt"

curl http://localhost:2323/index/Health
```

### Example response

```json
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
```