#include "ApiServer.h"

#include <sstream>
#include <iostream>

ApiServer::ApiServer() {}

void ApiServer::run(int port) {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/health")([this]() {
        return crow::response(200, buildHealthResponse());
    });
    CROW_ROUTE(app, "/indexHealth")([this]() {
        return crow::response(200, buildIndexHealthResponse());
    });
    CROW_ROUTE(app, "/search")([this](const crow::request& req) {
        const char* q = req.url_params.get("q");
        if (!q) {
            return crow::response(400, "{\"error\":\"missing query parameter 'q'\"}");
        }
        return crow::response(200, buildSearchResponse(q));
    });
    CROW_ROUTE(app, "/initalIndex").methods("POST"_method)([this](const crow::request& req) {
        const char* path = req.url_params.get("path");
        if (!path) {
            return crow::response(400, "{\"error\":\"missing query parameter 'path'\"}");
        }
        bool ok = buildInitalIndex(path);
        if (!ok) {
            return crow::response(400, "{\"status\":\"failed to build index\"}");
        }
        return crow::response(200, "{\"status\":\"index built\"}");
    });

    app.port(port).multithreaded().run();
}

std::string ApiServer::buildHealthResponse() const {
    std::string state = "Running";
    return "Server is currently : " + state;
}
bool ApiServer::buildInitalIndex(const std::string& path) {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".txt" || entry.path().extension() == ".md")) {
            files.push_back(entry.path().string());
        }
    }
    if (files.empty()) {
        return false;
    }

    engine.indexFiles(files);
    return true;
}
std::string ApiServer::buildSearchResponse(const std::string& query) const {
    std::vector<SearchResult> results = engine.search(query);
    std::ostringstream out;

    out << "{";
    out << "\"query\":\"" << escapeJson(query) << "\",";
    out << "\"count\":" << results.size() << ",";
    out << "\"results\":[";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        out << "{";
        out << "\"file\":\"" << escapeJson(result.file) << "\",";
        out << "\"score\":" << result.score << ",";
        out << "\"termMatches\":[";

        for (size_t j = 0; j < result.termMatches.size(); ++j) {
            const auto& termMatch = result.termMatches[j];
            out << "{";
            out << "\"term\":\"" << escapeJson(termMatch.term) << "\",";
            out << "\"occurrences\":[";

            for (size_t k = 0; k < termMatch.occurrences.size(); ++k) {
                const auto& occ = termMatch.occurrences[k];
                out << "{";
                out << "\"tokenPos\":" << occ.tokenPos << ",";
                out << "\"byteOffset\":" << occ.byteOffset << ",";
                out << "\"length\":" << occ.length;
                out << "}";

                if (k + 1 < termMatch.occurrences.size()) out << ",";
            }
            out << "]";
            out << "}";
            if (j + 1 < result.termMatches.size()) out << ",";
        }
        out << "]";
        out << "}";
        if (i + 1 < results.size()) out << ",";
    }
    out << "]";
    out << "}";
    return out.str();
}

std::string ApiServer::buildIndexHealthResponse() const {
    return "Index is currently at : " + std::to_string(engine.getTotalTerms()) + " terms.";
}

std::string ApiServer::escapeJson(const std::string& text) const {
    std::ostringstream out;
    for (char c : text) {
        switch (c) {
            case '\"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:   out << c;      break;
        }
    }
    return out.str();
}