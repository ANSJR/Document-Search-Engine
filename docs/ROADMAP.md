### Project Roadmap

## On Hold
- PDF reader modules

## Planned Features
# High Priority
- Phrase search (exact multi-word match)
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
- 

## Known Issues
- Prefix+prefix are fickel to work (Pattern Unknown)
- Index build slow for large files (needs parallelism)



- FIXEDish Found grammer bug where single quotation marks don't store due to it being unicode which is not char sized