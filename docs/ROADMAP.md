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

## Planned Features
# High Priority
- Save / load index to disk

# Mid Priority
- Parallelism

# Low Priority
- Implement wildcard search (ap*e -> apple, appliance)

## Experimental Ideas
- Web UI (React or simple HTML)
- Search suggestions (auto-complete)
- Return highlighted snippets (brown -> ... the quick **brown** fox jumps ...)
- Chunk/block indexing where files are split into chunks so only specific blocks need reindex when file is updated


## Known Issues
- Prefix+prefix are fickel to work (Pattern Unknown)
- Index build slow for large files (needs parallelism)
- FIXEDish Found grammer bug where single quotation marks don't store due to it being unicode which is not char sized