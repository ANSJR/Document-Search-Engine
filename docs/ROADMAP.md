### Project Roadmap

## Visualizer
# Index

old index = unordered_map<WORD, unordered_map <FILE, vector<POS> > > 
new index = unordered_map<WORD, unordered_map <FILE, WordLocation > > 
struct WordLocation {
    size_t tokenPos;
    size_t byteOffset;
    size_t length;
};

## On Hold
- PDF reader modules
- Add AND / OR / NOT operators within queury search
- Editor buffer search where memory is used for search of live edits
and only reindex the file when it is saved

## Planned

# High Priority
- generation-aware asynchronous incremental persistence system (Incremental index persistence) "Implemented asynchronous incremental index persistence using dirty-file tracking and generation-based stale job invalidation to avoid full-index rewrites and minimize memory overhead."
  - stale-job invalidation
- check if IndexSerializer shuts off if indexing of a new file starts
- Serialize index to disk with a fast index reload without rebuilding
- Cleaning up architecture between engine, search, indexing, and API layers


# Mid Priority
- Replace term/file strings keys in index to IDs in order to reduce expensive hashing in the hot paths.
- Memory usage profiling and optimization
- Binary index format
- Compressed posting lists maybe
- API pagination
- Structured JSON logging
- Dockerized deployment

# Low Priority
- Snippet/context extraction via byte offsets
- Improving result formatting of JSON search results for integration into future apps
- Unit tests using GoogleTest
- Integration tests for indexing and querying

## Experimental Ideas
- Web UI (React or simple HTML)
- Search suggestions (auto-complete)
- Return highlighted snippets (brown -> ... the quick **brown** fox jumps ...)
- Chunk/block indexing where files are split into chunks so only specific blocks need reindex when file is updated
- Test coverage for:
  - tokenization
  - ranking correctness
  - deletion
  - prefix search
  - edge cases
- CI pipeline with automated testing
- filtering of api GET /search?q=apple&type=pdf
- Rate limiting and request validation
- Compressed posting lists 


## Known Issues
- Prefix+prefix are fickel to work (Pattern Unknown)
- Index build slow for large files (needs parallelism)
- FIXEDish Found grammer bug where single quotation marks don't store due to it being unicode which is not char sized