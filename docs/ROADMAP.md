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


## Planned Features
# High Priority
- Incremental Indexing that uses unordered_map<FILE, unordered_set<WORD>> fileWords; to store words that each files uses and deletes files attached to said word in index and reindexes (this way a file reindex should only happen during a save in tandem with a editor buffer search)
- Editor buffer search where memory is used for search of live edits
and only reindex the file when it is saved

# Mid Priority
- Add AND / OR / NOT operators within queury search

# Low Priority
- BM25 ranking model
- Save / load index to disk
- Optimize context searching 


## Experimental Ideas
- Web UI (React or simple HTML)
- Implement wildcard search (ap*e -> apple, appliance)
- Search suggestions (auto-complete)
- Return highlighted snippets (brown -> ... the quick **brown** fox jumps ...)
- Chunk/block indexing where files are split into chunks so only specific blocks need reindex whem file is updated


## Known Issues
- Prefix+prefix are fickel to work (Pattern Unknown)
- Index build slow for large files (needs parallelism)



- FIXEDish Found grammer bug where single quotation marks don't store due to it being unicode which is not char sized