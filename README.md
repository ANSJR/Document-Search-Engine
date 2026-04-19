# Document Search Engine

A C++ document search engine for indexing and searching local text-based files, designed as a backend component that can later be exposed through a local API and can be integrated into other applications.

## Current Status

This project is currently in active development.

### What currently works
- Indexes local `.txt` and `.md` files from a directory
- Tokenizes and normalizes words for search
- Builds an inverted index of terms to files and match locations
- Stores positional metadata for matches
- Supports ranked search using TF-IDF-style scoring
- Supports prefix-based lookup using a Ternary Search Tree (TST)
- Includes a local HTTP API server using Crow
- `/health` endpoint is working
- `/search` endpoint is connected to the engine
- Project currently builds and runs locally from the command line

### In progress
- Returning fully structured JSON search results through the API
- Improving result formatting for integration into future apps
- Cleaning up architecture between engine, search, indexing, and API layers

### Planned
- File reindex/update for API use
- Snippet/context extraction via byte offsets
- Single-file incremental reindexing
- Multithreaded parallel index construction
- Integration into a future application

---

## Project Goal

The long-term goal of this project is to become a reusable local search backend that can be attached to other software or used stand alone as a highly efficient tool with performance put first.

---

## Current Search Data Model

The engine stores positional information for matched terms, including:
- token position
- byte offset
- token length

This is meant to support future features like:
- snippet generation
- match highlighting
- fast context extraction

---

## API Status

The project now includes a local API server built with Crow.

### Current endpoints
GET /health
- Confirms that the API server is running.
GET /search?q=...
- Runs a search query through the engine.
GET /indexHealth
- Returns basic index term information.
POST /initialIndex?path=...
- Builds the initial index from a specified directory of files.
POST /index/file?path=...
- Indexes a new file or reindex an existing file while performing DELETE actions on data that not in use by other files.
DELETE /index/file?path=...
- Eliminates file and associated terms(that are no longer used) from index.


### Example
curl http://localhost:2323/health
curl -X POST "http://localhost:2323/initialIndex?path=data"
curl http://localhost:2323/indexHealth
curl "http://localhost:2323/search?q=birthday"
curl -X POST "http://localhost:2323/index/file?path=dataTemp/BBcase.txt"
curl http://localhost:2323/indexHealth
curl "http://localhost:2323/search?q=birthday"
curl -X DELETE "http://localhost:2323/index/file?path=dataTemp/BBcase.txt"
curl http://localhost:2323/indexHealth
