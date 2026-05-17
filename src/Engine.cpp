
#include "../include/Engine.h"

Engine::Engine() {
    loadConfig("config/config.json");
    std::filesystem::create_directories(config.indexBin);
    loadSerializedIndex();
    serializerWorker = std::thread(&Engine::serializerLoop, this);
}
bool Engine::saveConfig(const std::filesystem::path& configPath) const {
    try {
        if (configPath.has_parent_path()) {
            std::filesystem::create_directories(
                configPath.parent_path()
            );
        }

        std::ofstream out(configPath);

        if (!out) return false;

        out << "{\n";
        out << R"(    "indexBin": ")" << config.indexBin.generic_string() << "\"\n";
        out << "}\n";
        return out.good();
    }
    catch (const std::exception&) {
        return false;
    }
}
bool Engine::loadConfig(const std::filesystem::path& configPath) {
    try {
        if (!std::filesystem::exists(configPath)) return false;

        std::ifstream in(configPath);
        if (!in) return false;

        std::stringstream buffer;
        buffer << in.rdbuf();

        
        auto json = crow::json::load(buffer.str());

        if (!json) return false;
        if (json.has("indexBin")) {
            std::filesystem::path loadedPath(std::string(json["indexBin"].s()));
            if (!loadedPath.empty()) {
                std::filesystem::create_directories(loadedPath);

                config.indexBin = std::filesystem::weakly_canonical(loadedPath);
            }
        }
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}
void Engine::serializerLoop() {
    while (true) {
        DirtyJob job;
        {
            std::unique_lock lock(dirtyMutex);
            dirtyCV.wait(lock, [&] {return stopWorker || !dirtyQueue.empty();}); // engine is shutting down or items in queue
            if (stopWorker && dirtyQueue.empty()) break;
            job = dirtyQueue.front();
            dirtyQueue.pop();
        }

        // std::cout << "Serializing: " << job.file << '\n';
        std::filesystem::path currentIndexBin;
        {
            std::lock_guard lock(pathMutex);
            currentIndexBin = config.indexBin;
        }
        std::filesystem::path output = currentIndexBin / (job.file.filename().string() + ".bin");
        output = output.lexically_normal();
        {
            std::shared_lock lock(engineMutex);
            if (job.generation != indexer.getFileGen(job.file)) continue;
            IndexSerializer::save(indexer, job.file, output);
        }
    }
}
void Engine::loadSerializedIndex() {
    IndexSerializer::load(indexer, tst, config.indexBin);
}
void Engine::indexFiles(const std::vector<std::filesystem::path>& files) {
    std::unique_lock lock(engineMutex);
    indexer.buildIndex(files, tst);
    {
        std::lock_guard dirtyLock(dirtyMutex);
        for (const auto& file : files) {
            dirtyQueue.push({file, indexer.getFileGen(file)});
        }
    }
    dirtyCV.notify_one();
}
void Engine::indexFile(const std::filesystem::path& filePath) {
    std::unique_lock lock(engineMutex);
    indexer.buildIndex(filePath, tst);
    {
        std::lock_guard dirtyLock(dirtyMutex);
        dirtyQueue.push({filePath, indexer.getFileGen(filePath)});
    }
    dirtyCV.notify_one();
}
bool Engine::modifyIndexBinPath(const std::filesystem::path& newPath) {
    try {
        if (newPath.empty()) return false;
        if (!std::filesystem::exists(newPath)) std::filesystem::create_directories(newPath);

        std::filesystem::path normalized = std::filesystem::weakly_canonical(newPath);
        if (!std::filesystem::is_directory(normalized)) return false;
        if (normalized == normalized.root_path()) return false;

        {
            std::lock_guard lock(dirtyMutex);
            stopWorker = true;
        }
        dirtyCV.notify_one();
        if (serializerWorker.joinable()) {
            serializerWorker.join();
        }

        std::filesystem::path oldIndexBin;
        {
            std::lock_guard lock(pathMutex);
            oldIndexBin = config.indexBin;
        }

        for (const auto& entry : std::filesystem::directory_iterator(oldIndexBin)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".bin") continue;

            std::filesystem::path destination = normalized / entry.path().filename();
            std::filesystem::rename(entry.path(), destination);
        }

        {
            std::lock_guard lock(pathMutex);
            config.indexBin = normalized;
        }
        {
            std::lock_guard lock(dirtyMutex);
            stopWorker = false;
        }
        serializerWorker = std::thread(&Engine::serializerLoop, this);

        return true;
    }
    catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}
void Engine::deleteTermFromFile(const std::filesystem::path& filePath) {
    std::unique_lock lock(engineMutex);
    indexer.removeFileFromIndex(filePath, tst);
}
std::vector<SearchResult> Engine::search(const std::string& query) const {
    std::shared_lock lock(engineMutex);
    const auto& index = indexer.getIndex();
    Searcher searcher(index, tst);
    auto results = searcher.search(query);
    std::vector<SearchResult> finalResults;
    if (results.empty()) std::cout << "EMPTY RESULT";

    // if (results.begin()->first != tokenizer.isolateLastToken(query)) {
    //     std::cout << "\nPERFORMING PREFIX SEARCH\n\n";
    // }
    std::unordered_map<std::filesystem::path, SearchResult> fileMap;
    for (const auto& [word, fileMapInner] : results) {
        for (const auto& [file, locations] : fileMapInner) {
            // create SearchResult if none exist
            if (fileMap.find(file) == fileMap.end()) {
                fileMap[file] = SearchResult{file, 0.0,{}};
            }
            TermMatch termMatch;
            termMatch.term = word;
            for (const auto& loc : locations) {
                MatchOccurrence occ;
                occ.tokenPos = loc.tokenPos;
                occ.byteOffset = loc.byteOffset;
                occ.length = word.size();
                termMatch.occurrences.push_back(occ);
            }
            // add term to file
            fileMap[file].termMatches.push_back(termMatch);
            fileMap[file].score += indexer.computeScore(file, word);
        }
    }
    finalResults.reserve(fileMap.size());
    for (const auto& [file, result] : fileMap) {
        finalResults.push_back(result);
    }

    std::sort(finalResults.begin(), finalResults.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.score > b.score;
        });
    // printSearchResults(finalResults);
    return finalResults;
}
int Engine::getTotalIndexTerms() const {
    std::shared_lock lock(engineMutex);
    return indexer.getTotalIndexTerms();
}
int Engine::getTotalTreeTerms() const {
    return tst.countWords();
}
Engine::~Engine() {
    {
        std::lock_guard lock(dirtyMutex);
        stopWorker = true;
    }

    dirtyCV.notify_one();
    if (serializerWorker.joinable()) {
        serializerWorker.join();
    }
}

void printSearchResults(const std::vector<SearchResult>& results) {
    if (results.empty()) {
        std::cout << "No results found.\n";
        return;
    }

    for (const auto& result : results) {
        std::cout << "FILE: " << result.file << "\n";
        std::cout << "SCORE: " << result.score << "\n";

        for (const auto& termMatch : result.termMatches) {
            std::cout << "  TERM: " << termMatch.term << "\n";
            for (const auto& occ : termMatch.occurrences) {
                std::cout << "    (tokenPos: " << occ.tokenPos
                          << ", byteOffset: " << occ.byteOffset
                          << ", length: " << occ.length << ")\n";
            }
        }
        std::cout << "-------------------------\n";
    }
}