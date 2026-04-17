#ifndef APISERVER_H
#define APISERVER_H

#include <string>
#include <vector>
#include "Engine.h"
#include "SearchTypes.h"
#include "crow.h"

class ApiServer {
private:
    Engine engine;

public:
    ApiServer();

    void run(int port);

private:
    std::string buildHealthResponse() const;
    bool buildInitialIndex(const std::string& path);
    bool addFileToIndex(const std::string& filePath);
    std::string buildSearchResponse(const std::string& query) const;
    std::string buildIndexHealthResponse() const;
    std::string escapeJson(const std::string& text) const;
};

#endif