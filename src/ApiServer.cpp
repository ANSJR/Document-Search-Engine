#include "ApiServer.h"

#include <sstream>
#include <iostream>

ApiServer::ApiServer() {}

void ApiServer::run(int port) {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/health")([this]() {
        return crow::response(200, buildHealthResponse());
    });

    CROW_ROUTE(app, "/search")([this](const crow::request& req) {
        const char* q = req.url_params.get("q");
        if (!q) {
            return crow::response(400, "{\"error\":\"missing query parameter 'q'\"}");
        }
        return crow::response(200, buildSearchResponse(q));
    });

    app.port(port).multithreaded().run();
}

std::string ApiServer::buildHealthResponse() const {
    std::string state = "Running";
    return "Server is currently : " + state;
}

std::string ApiServer::buildSearchResponse(const std::string& query) const {
    std::vector<SearchResult> results = engine.search(query);

    return "Not fully implemented yet";
}

std::string ApiServer::buildIndexResponse() const {
    return "Not fully implemented yet";
}
