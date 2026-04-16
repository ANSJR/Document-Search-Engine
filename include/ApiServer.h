#ifndef APISERVER_H
#define APISERVER_H

#include <string>
#include <vector>
#include "Engine.h"
#include "crow.h"

class ApiServer {
private:
    Engine engine;

public:
    ApiServer();

    void run(int port);

private:
    std::string buildHealthResponse() const;
    std::string buildSearchResponse(const std::string& query) const;
    std::string buildIndexResponse() const;
    
};

#endif